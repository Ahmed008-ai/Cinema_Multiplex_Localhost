/* CINÉ frontend: the browser handles presentation; all booking rules live in C. */
let appState = null;
let currentPage = 'overview';
let selectedShowId = null;
let selectedSeat = null;
let bookingForm = { customerId: '', comboId: 0, quantity: 1, promoCode: '' };
let searchPopover = null;

const $ = (selector, root = document) => root.querySelector(selector);
const money = value => 'Rs. ' + Number(value || 0).toLocaleString('en-PK', { minimumFractionDigits: 0, maximumFractionDigits: 2 });
const esc = value => String(value ?? '').replace(/[&<>"']/g, ch => ({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[ch]));
const movieArtClass = id => `poster-${Math.abs(Number(id) || 0) % 6}`;
const initials = name => String(name || '?').split(/\s+/).slice(0,2).map(part => part[0] || '').join('').toUpperCase();
const fmtDate = value => { if (!value) return ''; const d = new Date(value + 'T12:00:00'); return d.toLocaleDateString('en-US',{month:'short',day:'numeric',year:'numeric'}); };
const todayISO = () => { const d = new Date(); return `${d.getFullYear()}-${String(d.getMonth()+1).padStart(2,'0')}-${String(d.getDate()).padStart(2,'0')}`; };
const tomorrowISO = () => { const d = new Date(); d.setDate(d.getDate()+1); return `${d.getFullYear()}-${String(d.getMonth()+1).padStart(2,'0')}-${String(d.getDate()).padStart(2,'0')}`; };

async function apiGet(path) {
  const response = await fetch(path, { cache: 'no-store' });
  const data = await response.json();
  if (!response.ok || data.ok === false) throw new Error(data.message || 'Request failed');
  return data;
}
async function apiPost(action, payload) {
  const response = await fetch(`/api/${action}`, { method:'POST', headers:{'Content-Type':'application/json'}, body:JSON.stringify(payload) });
  const data = await response.json();
  if (!response.ok || !data.ok) { const error = new Error(data.message || 'Request failed'); error.data = data; throw error; }
  return data;
}
async function refreshData(silent = false) {
  try {
    appState = await apiGet('/api/state');
    renderCurrentPage();
    renderSidebarDate();
    if (!silent) toast('Your cinema data is up to date.', 'success');
  } catch (error) {
    toast(error.message, 'error');
  }
}
function renderSidebarDate() {
  const el = $('#today-chip');
  if (el) el.textContent = new Date().toLocaleDateString('en-US',{weekday:'short',day:'2-digit',month:'short',year:'numeric'}).toUpperCase();
}
function toast(message, type = 'success') {
  const stack = $('#toast-stack');
  const item = document.createElement('div');
  item.className = `toast ${type}`;
  item.textContent = message;
  stack.appendChild(item);
  setTimeout(() => item.remove(), 4200);
}
function setPage(page) {
  currentPage = page;
  document.querySelectorAll('.page').forEach(el => el.classList.toggle('active', el.id === `page-${page}`));
  document.querySelectorAll('.nav-link').forEach(el => el.classList.toggle('active', el.dataset.page === page));
  const labels = {overview:'Overview',movies:'Movie library',shows:'Showtimes',booking:'Ticket desk',customers:'Customers',reports:'Insights'};
  $('#crumb-current').textContent = labels[page] || 'Overview';
  renderCurrentPage();
  window.scrollTo({top:0,behavior:'smooth'});
}
window.setPage = setPage;
window.refreshData = refreshData;
function renderCurrentPage() {
  if (!appState) return;
  if (currentPage === 'overview') renderOverview();
  if (currentPage === 'movies') renderMovies();
  if (currentPage === 'shows') renderShows();
  if (currentPage === 'booking') renderBooking();
  if (currentPage === 'customers') renderCustomers();
  if (currentPage === 'reports') renderReports();
}
function statCard(label, value, note, icon, color) {
  return `<article class="stat-card"><div class="stat-top"><span>${label}</span><span class="stat-icon ${color}">${icon}</span></div><div class="stat-value">${value}</div><div class="stat-note">${note}</div></article>`;
}
function activeBookings() { return appState.bookings.filter(item => item.active); }
function totalRevenue() { return appState.movies.reduce((sum,movie) => sum + Number(movie.revenue || 0), 0); }
function sortedShows() { return [...appState.shows].sort((a,b) => (a.date+a.time).localeCompare(b.date+b.time)); }
function renderOverview() {
  const active = activeBookings();
  const occupancy = appState.shows.length ? appState.shows.reduce((sum,show)=>sum+show.occupancy,0)/appState.shows.length : 0;
  $('#overview-stats').innerHTML = [
    statCard('MOVIES IN LIBRARY', appState.movies.length, 'Titles available to schedule', '▣', 'blue'),
    statCard('SCHEDULED SHOWS', appState.shows.length, `${appState.meta.screenCount} physical screens · fixed capacity`, '▦', 'mint'),
    statCard('ACTIVE BOOKINGS', active.length, `${appState.customers.length} registered customers`, '▤', 'gold'),
    statCard('BOX OFFICE REVENUE', money(totalRevenue()), `${occupancy.toFixed(0)}% average seat occupancy`, '↗', 'pink')
  ].join('');
  const upcoming = sortedShows().slice(0,5);
  $('#overview-schedule').innerHTML = upcoming.length ? `<div class="screen-show-list">${upcoming.map(show=>`<div class="screen-show-row"><div class="show-time">${esc(show.time)}</div><div class="table-primary"><span class="show-poster-mini ${movieArtClass(show.movieId)}">✦</span><div class="show-movie"><strong>${esc(show.movieTitle)}</strong><small>${fmtDate(show.date)} · ${show.duration} min</small></div></div><span class="screen-label">Screen ${show.screenId+1}</span><span class="status-pill ${show.occupancy>=100?'waiting':'confirmed'}">${show.occupancy>=100?'Sold out':`${show.occupancy.toFixed(0)}% full`}</span></div>`).join('')}</div>` : emptyTable('No screenings scheduled yet. Add your first show to get started.');
  const screens = [0,1,2].map(id=>{
    const shows = appState.shows.filter(show=>show.screenId===id);
    const avg = shows.length ? shows.reduce((sum,s)=>sum+s.occupancy,0)/shows.length : 0;
    return `<div class="occupancy-item"><div class="occupancy-item-head"><strong>Screen ${id+1}</strong><span>${shows.length} show${shows.length===1?'':'s'} · ${avg.toFixed(0)}%</span></div><div class="progress"><i style="width:${Math.min(avg,100)}%"></i></div></div>`;
  }).join('');
  $('#overview-occupancy').innerHTML = `<div class="occupancy-list">${screens}</div>`;
  $('#overview-bookings').innerHTML = bookingTableRows(appState.bookings.slice().reverse().slice(0,5));
}
function emptyTable(message) { return `<div class="empty-table">${esc(message)}</div>`; }
function bookingTableRows(rows) {
  if (!rows.length) return emptyTable('No booking activity yet. Confirmed tickets will appear here.');
  return `<div class="table-scroll"><table><thead><tr><th>Booking</th><th>Guest</th><th>Movie / show</th><th>Seat</th><th>Amount</th><th>Status</th></tr></thead><tbody>${rows.map(item=>`<tr><td><strong>${esc(item.id)}</strong></td><td>${esc(item.customerName)}</td><td>${esc(item.movieTitle)}<br><small style="color:#a1a6b3">${fmtDate(item.date)} · ${esc(item.time)}</small></td><td>${esc(item.seat)}</td><td><strong>${money(item.total)}</strong></td><td><span class="status-pill ${item.active?'confirmed':'cancelled'}">${item.active?'Confirmed':'Cancelled'}</span></td></tr>`).join('')}</tbody></table></div>`;
}
function renderMovies() {
  const filter = ($('#movie-filter')?.value || '').trim().toLowerCase();
  const list = appState.movies.filter(movie=>movie.title.toLowerCase().includes(filter));
  $('#movie-count-label').textContent = `${appState.movies.length} title${appState.movies.length===1?'':'s'} in your catalogue`;
  $('#movie-grid').innerHTML = list.length ? list.map(movie=>{
    const showCount = appState.shows.filter(show=>show.movieId===movie.id).length;
    return `<article class="movie-card"><div class="movie-art ${movieArtClass(movie.id)}"><div class="poster-symbol">${['◈','✧','◉','✦','◇','☼'][movie.id%6]}</div><div class="poster-label"><small>NOW IN YOUR LIBRARY</small><strong>${esc(movie.title)}</strong></div></div><div class="movie-card-body"><div class="movie-meta"><span>${movie.duration} MINUTES</span><span>FEATURE FILM</span></div><div class="movie-card-foot"><div class="movie-revenue"><small>BOX OFFICE REVENUE</small><strong>${money(movie.revenue)}</strong></div><span class="movie-shows">${showCount} scheduled show${showCount===1?'':'s'}</span></div></div></article>`;
  }).join('') : emptyTable('No movies match that search.');
}
function renderShows() {
  $('#screen-summary').innerHTML = [0,1,2].map(id=>{
    const list = appState.shows.filter(show=>show.screenId===id);
    const occupancy = list.length ? list.reduce((sum,s)=>sum+s.occupancy,0)/list.length : 0;
    return `<article class="screen-card"><div class="screen-card-head"><strong>Screen 0${id+1}</strong><span>${list.length} SHOWS</span></div><p>${list.length ? `${list.length} scheduled screening${list.length===1?'':'s'} across your programme` : 'Ready for its next screening'}</p><div class="progress"><i style="width:${Math.min(occupancy,100)}%"></i></div><div class="screen-card-foot"><span>Average occupancy</span><strong>${occupancy.toFixed(0)}%</strong></div></article>`;
  }).join('');
  const rows = sortedShows();
  $('#shows-table').innerHTML = rows.length ? `<table><thead><tr><th>Show ID</th><th>Movie</th><th>Date</th><th>Start time</th><th>Screen</th><th>Seats booked</th><th>Occupancy</th></tr></thead><tbody>${rows.map(show=>`<tr><td><strong>#${String(show.id+1).padStart(3,'0')}</strong></td><td><div class="table-primary"><span class="table-poster ${movieArtClass(show.movieId)}">✦</span><strong>${esc(show.movieTitle)}</strong></div></td><td>${fmtDate(show.date)}</td><td><strong>${esc(show.time)}</strong></td><td><span class="soft-tag">Screen ${show.screenId+1}</span></td><td>${show.booked} / ${show.total}</td><td><strong>${show.occupancy.toFixed(0)}%</strong></td></tr>`).join('')}</tbody></table>` : emptyTable('Your schedule is clear. Create a showtime to begin.');
}
function renderBooking() {
  const shows = sortedShows();
  if (selectedShowId !== null && !appState.shows.some(s=>s.id===selectedShowId)) selectedShowId = null;
  const selected = appState.shows.find(s=>s.id===selectedShowId) || null;
  if (!selected && shows.length) selectedShowId = shows[0].id;
  const show = appState.shows.find(s=>s.id===selectedShowId) || null;
  $('#booking-show-picker').innerHTML = shows.length ? `<select class="select-show" id="booking-show-select" onchange="selectBookingShow(this.value)"><option value="">Choose a movie and screening…</option>${shows.map(s=>`<option value="${s.id}" ${show&&show.id===s.id?'selected':''}>${esc(s.movieTitle)} · ${fmtDate(s.date)} · ${esc(s.time)} · Screen ${s.screenId+1}</option>`).join('')}</select>` : emptyTable('No shows have been scheduled. Add a show before taking bookings.');
  if (show) renderSeatMap(show); else $('#seat-map-area').innerHTML = `<div class="empty-state"><span class="empty-icon">▤</span><strong>Select a screening first</strong><p>Your seat map will appear here.</p></div>`;
  renderBookingSummary(show);
  // Show recent transactions with a cancellation action for active tickets.
  $('#booking-table').innerHTML = appState.bookings.length ? `<div class="table-scroll"><table><thead><tr><th>Booking</th><th>Customer</th><th>Movie</th><th>Seat</th><th>Total</th><th>Status</th><th>Action</th></tr></thead><tbody>${appState.bookings.slice().reverse().map(b=>`<tr><td><strong>${esc(b.id)}</strong></td><td>${esc(b.customerName)}</td><td>${esc(b.movieTitle)}<br><small style="color:#a1a6b3">${fmtDate(b.date)} · ${esc(b.time)}</small></td><td>${esc(b.seat)}</td><td><strong>${money(b.total)}</strong></td><td><span class="status-pill ${b.active?'confirmed':'cancelled'}">${b.active?'Confirmed':'Cancelled'}</span></td><td>${b.active?`<button class="text-button" data-action="cancel" data-id="${esc(b.id)}">Cancel</button>`:'—'}</td></tr>`).join('')}</tbody></table></div>` : emptyTable('No tickets have been booked yet.');
}
function selectBookingShow(value) {
  selectedShowId = value === '' ? null : Number(value);
  selectedSeat = null;
  bookingForm = {customerId:'',comboId:0,quantity:1,promoCode:''};
  renderBooking();
}
window.selectBookingShow = selectBookingShow;
function renderSeatMap(show) {
  if (show.occupancy >= 100) {
    $('#seat-map-area').innerHTML = `<div class="waitlist-box"><strong>This screening is fully booked</strong><p>All ${show.total} seats are currently allocated. You can add a guest to the waiting list; a cancelled seat will be promoted automatically.</p><div class="form-row"><div class="field"><label>Customer</label><select id="waitlist-customer" class="select-show"><option value="">Choose a customer…</option>${appState.customers.map(c=>`<option value="${c.id}">${esc(c.name)} · ${esc(c.phone)}</option>`).join('')}</select></div><div class="field" style="align-self:end"><button class="btn btn-primary" data-action="waitlist" data-show="${show.id}">Join waiting list</button></div></div></div>`;
    return;
  }
  const rows = show.seats.map((seatRow,r)=>`<div class="seat-row"><span class="row-label">${String.fromCharCode(65+r)}</span>${seatRow.map((status,c)=>`<button class="seat-button ${status?'booked':(selectedSeat&&selectedSeat.row===r&&selectedSeat.col===c?'selected':'')}" ${status?'disabled':''} data-action="seat" data-row="${r}" data-col="${c}" title="Row ${String.fromCharCode(65+r)}, seat ${c+1} · ${r<2?'Silver':r<4?'Gold':'Platinum'}">${c+1}</button>`).join('')}</div>`).join('');
  $('#seat-map-area').innerHTML = `<div class="seat-legend"><span><i class="seat-swatch free"></i> Available</span><span><i class="seat-swatch selected"></i> Selected</span><span><i class="seat-swatch taken"></i> Booked</span></div><div class="screen-visual">SCREEN</div><div class="seat-grid">${rows}</div><div class="seat-category-note">Rows A–B · Silver &nbsp; / &nbsp; Rows C–D · Gold &nbsp; / &nbsp; Rows E–F · Platinum</div>`;
}
function renderBookingSummary(show) {
  const selected = selectedSeat && show ? `${String.fromCharCode(65+selectedSeat.row)}${selectedSeat.col+1}` : 'Not selected';
  const customerOptions = appState.customers.map(c=>`<option value="${c.id}" ${String(bookingForm.customerId)===String(c.id)?'selected':''}>${esc(c.name)} · ${esc(c.phone)}</option>`).join('');
  const comboOptions = appState.combos.map(c=>`<option value="${c.id}" ${Number(bookingForm.comboId)===c.id?'selected':''}>${esc(c.name)} · ${money(c.price)}</option>`).join('');
  let ticket = 0, food = 0, discount = 0;
  if (show && selectedSeat) {
    const category = selectedSeat.row < 2 ? 0 : selectedSeat.row < 4 ? 1 : 2;
    ticket = show.prices[category];
    const combo = appState.combos.find(c=>c.id===Number(bookingForm.comboId));
    food = combo ? combo.price * Number(bookingForm.quantity || 0) : 0;
    const promo = appState.promos.find(p=>p.code.toLowerCase()===String(bookingForm.promoCode).trim().toLowerCase());
    if (promo) discount = (ticket+food)*promo.percent/100;
  }
  const total = Math.max(0,ticket+food-discount);
  $('#booking-summary').innerHTML = `<div class="summary-kicker">YOUR ORDER</div><div class="summary-title">Booking summary</div><div class="summary-sub">${show?`${esc(show.movieTitle)} · Screen ${show.screenId+1}`:'Select a show to begin'}</div><hr class="summary-divider"><div class="summary-line"><span>Screening</span><strong>${show?`${fmtDate(show.date)} · ${esc(show.time)}`:'—'}</strong></div><div class="summary-line"><span>Selected seat</span><strong>${selected}</strong></div><div class="summary-line"><span>Ticket</span><strong>${money(ticket)}</strong></div><div class="field" style="margin-top:17px"><label>Customer</label><select class="select-show" id="booking-customer" onchange="bookingForm.customerId=this.value">${customerOptions ? '<option value="">Choose a registered customer…</option>'+customerOptions : '<option value="">Register a customer first</option>'}</select></div><div class="field" style="margin-top:12px"><label>Food & refreshments</label><select class="select-show" id="booking-combo" onchange="bookingForm.comboId=Number(this.value);renderBookingSummary(appState.shows.find(s=>s.id===selectedShowId))"><option value="0">No combo</option>${comboOptions}</select></div><div class="form-row" style="margin-top:12px"><div class="field"><label>Combo quantity</label><input id="booking-quantity" type="number" min="1" max="10" value="${Number(bookingForm.quantity)||1}" onchange="bookingForm.quantity=Math.max(1,Math.min(10,Number(this.value)||1));renderBookingSummary(appState.shows.find(s=>s.id===selectedShowId))"></div><div class="field"><label>Promo code</label><input id="booking-promo" value="${esc(bookingForm.promoCode)}" placeholder="Optional" oninput="bookingForm.promoCode=this.value"></div></div><hr class="summary-divider"><div class="summary-line"><span>Food & drinks</span><strong>${money(food)}</strong></div><div class="summary-line"><span>Discount</span><strong style="color:#2eaa81">− ${money(discount)}</strong></div><div class="summary-total"><span>Total payable</span><strong>${money(total)}</strong></div><div class="summary-note">Dynamic ticket pricing is calculated by the C backend using day type, showtime, seat category and current demand.</div><button class="btn btn-primary summary-submit" data-action="book" ${!show||!selectedSeat?'disabled':''}>Confirm booking <span>→</span></button>`;
}
function renderCustomers() {
  const filter = ($('#customer-filter')?.value || '').toLowerCase().trim();
  const list = appState.customers.filter(c=>c.name.toLowerCase().includes(filter)||c.phone.includes(filter));
  const gold = appState.customers.filter(c=>c.tier==='Gold').length;
  $('#customer-stats').innerHTML = [statCard('REGISTERED GUESTS',appState.customers.length,'Unique customer profiles','♙','blue'),statCard('GOLD MEMBERS',gold,'Guests with 50+ loyalty points','✦','gold'),statCard('ACTIVE BOOKINGS',activeBookings().length,'Current confirmed tickets','▤','mint')].join('');
  $('#customers-table').innerHTML = list.length ? `<table><thead><tr><th>Customer</th><th>Phone number</th><th>Loyalty points</th><th>Membership</th><th>Active tickets</th></tr></thead><tbody>${list.map(c=>`<tr><td><div class="customer-name-cell"><span class="customer-avatar">${esc(initials(c.name))}</span><strong>${esc(c.name)}</strong></div></td><td>${esc(c.phone)}</td><td><strong>${c.points}</strong> pts</td><td><span class="tier ${c.tier.toLowerCase()}">${c.tier}</span></td><td>${c.activeBookings}</td></tr>`).join('')}</tbody></table>` : emptyTable('No customer profiles match your search.');
}
function renderReports() {
  $('#report-stats').innerHTML = [statCard('TOTAL REVENUE',money(totalRevenue()),'Net collected across active tickets','↗','blue'),statCard('ACTIVE TICKETS',activeBookings().length,'Confirmed bookings','▤','mint'),statCard('CUSTOMERS',appState.customers.length,'Registered audience','♙','gold'),statCard('SHOWS',appState.shows.length,'Across three fixed screens','▦','pink')].join('');
  const movies = [...appState.movies].sort((a,b)=>b.revenue-a.revenue);
  const maxRevenue = Math.max(1,...movies.map(m=>m.revenue));
  $('#revenue-chart').innerHTML = movies.length ? `<div class="bar-chart">${movies.map(m=>`<div class="bar-column"><b>${money(m.revenue)}</b><i style="height:${Math.max(4,m.revenue/maxRevenue*115)}px"></i><small>${esc(m.title.length>11?m.title.slice(0,10)+'…':m.title)}</small></div>`).join('')}</div>` : emptyTable('Revenue appears here after tickets are booked.');
  const shows = [...appState.shows].sort((a,b)=>b.occupancy-a.occupancy).slice(0,7);
  $('#occupancy-chart').innerHTML = shows.length ? `<div class="rank-list">${shows.map((s,i)=>`<div class="rank-row"><span class="rank-num">${String(i+1).padStart(2,'0')}</span><div class="rank-main"><strong>${esc(s.movieTitle)} · Screen ${s.screenId+1}</strong><div class="progress"><i style="width:${s.occupancy}%"></i></div></div><span class="rank-value">${s.occupancy.toFixed(0)}%</span></div>`).join('')}</div>` : emptyTable('Occupancy reports appear when shows are scheduled.');
}
function openModal(title, subtitle, body) {
  $('#modal-card').innerHTML = `<div class="modal-top"><div><h2 id="modal-title">${title}</h2><p>${subtitle}</p></div><button class="modal-close" onclick="closeModal()" aria-label="Close">×</button></div>${body}`;
  $('#modal-backdrop').classList.add('open');
}
function closeModal() { $('#modal-backdrop').classList.remove('open'); }
window.closeModal = closeModal;
function openMovieModal() {
  openModal('Add a movie','Add a title to your cinema catalogue.',`<form class="modal-form" id="movie-form"><div class="field"><label>Movie title</label><input name="title" maxlength="49" placeholder="e.g. The Grand Adventure" required></div><div class="field"><label>Runtime (minutes)</label><input name="duration" type="number" min="30" max="300" value="120" required></div><div class="modal-actions"><button type="button" class="btn btn-quiet" onclick="closeModal()">Cancel</button><button class="btn btn-primary">Save movie <span>→</span></button></div></form>`);
  $('#movie-form').addEventListener('submit', async e=>{e.preventDefault();const f=new FormData(e.currentTarget);try{await apiPost('movies',{title:f.get('title').trim(),duration:Number(f.get('duration'))});closeModal();await refreshData(true);setPage('movies');toast('Movie added to your library.');}catch(err){toast(err.message,'error');}});
}
window.openMovieModal = openMovieModal;
function openCustomerModal() {
  openModal('Register a guest','Create a customer profile and start their loyalty journey.',`<form class="modal-form" id="customer-form"><div class="field"><label>Full name</label><input name="name" maxlength="39" placeholder="Guest name" required></div><div class="field"><label>Phone number</label><input name="phone" inputmode="numeric" minlength="7" maxlength="15" placeholder="Digits only · 7–15 characters" required></div><div class="modal-actions"><button type="button" class="btn btn-quiet" onclick="closeModal()">Cancel</button><button class="btn btn-primary">Create profile <span>→</span></button></div></form>`);
  $('#customer-form').addEventListener('submit',async e=>{e.preventDefault();const f=new FormData(e.currentTarget);try{await apiPost('customers',{name:f.get('name').trim(),phone:f.get('phone').trim()});closeModal();await refreshData(true);setPage('customers');toast('Customer profile created.');}catch(err){toast(err.message,'error');}});
}
window.openCustomerModal = openCustomerModal;
function openShowModal() {
  if (!appState.movies.length) { toast('Add a movie before scheduling a show.','error');setPage('movies');return; }
  const options = appState.movies.map(m=>`<option value="${m.id}">${esc(m.title)} · ${m.duration} min</option>`).join('');
  openModal('Schedule a screening','The cinema has three fixed screens and a 20-minute cleaning interval.',`<form class="modal-form" id="show-form"><div class="field"><label>Movie</label><select name="movieId" required>${options}</select></div><div class="form-row"><div class="field"><label>Show date</label><input name="date" type="date" min="${todayISO()}" value="${tomorrowISO()}" required></div><div class="field"><label>Start time</label><input name="time" type="time" value="18:30" required></div></div><div class="form-row"><div class="field"><label>Day type</label><select name="dayType"><option value="0">Weekday</option><option value="1">Weekend</option></select></div><div class="field"><label>Screen assignment</label><select name="screen"><option value="-1">Automatic assignment</option><option value="0">Screen 1</option><option value="1">Screen 2</option><option value="2">Screen 3</option></select></div></div><div class="modal-message">A show cannot overlap another movie on the same screen. If all screens are busy, the backend will suggest the next available time.</div><div class="modal-actions"><button type="button" class="btn btn-quiet" onclick="closeModal()">Cancel</button><button class="btn btn-primary">Schedule show <span>→</span></button></div></form>`);
  $('#show-form').addEventListener('submit',async e=>{e.preventDefault();const f=new FormData(e.currentTarget);const [hh,mm]=String(f.get('time')).split(':').map(Number);try{const result=await apiPost('shows',{movieId:Number(f.get('movieId')),date:f.get('date'),minutes:hh*60+mm,dayType:Number(f.get('dayType')),preferredScreen:Number(f.get('screen'))});closeModal();await refreshData(true);setPage('shows');toast(`Show scheduled on Screen ${result.screen}.`);}catch(err){const suggestion=err.message;toast(suggestion,'error');const msg=$('.modal-message');if(msg)msg.textContent=suggestion;}});
}
window.openShowModal = openShowModal;
async function submitBooking() {
  if (!selectedSeat || selectedShowId===null) return toast('Select a screening and a seat first.','error');
  const customerId = Number($('#booking-customer')?.value || bookingForm.customerId);
  if (!customerId) return toast('Choose a registered customer.','error');
  const comboId = Number($('#booking-combo')?.value ?? bookingForm.comboId);
  const quantity = comboId ? Number($('#booking-quantity')?.value || bookingForm.quantity || 1) : 0;
  const promoCode = $('#booking-promo')?.value.trim() || '';
  try {
    const result = await apiPost('bookings',{customerId,showId:selectedShowId,row:selectedSeat.row,col:selectedSeat.col,comboId,quantity,promoCode:promoCode||'NONE'});
    const show = appState.shows.find(s=>s.id===selectedShowId);
    const seatName = `${String.fromCharCode(65+selectedSeat.row)}${selectedSeat.col+1}`;
    selectedSeat = null; bookingForm = {customerId:'',comboId:0,quantity:1,promoCode:''};
    await refreshData(true);
    setPage('booking');
    showReceipt(result.bookingId,show,seatName,result);
  } catch(error) { toast(error.message,'error'); await refreshData(true); }
}
function showReceipt(bookingId, show, seatName, result) {
  const current = appState.bookings.find(b=>b.id===bookingId);
  openModal('Booking confirmed','Your guest is all set for the show.',`<div class="receipt-card"><div class="receipt-brand">CINÉ<span>.</span> <small>ADMIT ONE</small></div><h3>${esc(current?.movieTitle || show?.movieTitle || 'Cinema screening')}</h3><p>${current?fmtDate(current.date):''} · ${esc(current?.time || '')} · Screen ${current?.screen || ''}</p><div class="receipt-ticket"><div><small>BOOKING ID</small><strong>${esc(bookingId)}</strong></div><div><small>SEAT</small><strong>${esc(seatName)}</strong></div></div><div class="receipt-total"><span>Amount paid</span><strong>${money(result.total)}</strong></div><div class="receipt-note">Please arrive 15 minutes before the screening. Enjoy the show.</div></div><div class="modal-actions"><button class="btn btn-quiet" onclick="closeModal()">Close</button><button class="btn btn-primary" onclick="window.print()">Print receipt ↗</button></div>`);
}
async function cancelBooking(id) {
  if (!confirm(`Cancel booking ${id}? The refund will follow the project's time-based refund rules.`)) return;
  try { const result=await apiPost('cancel',{bookingId:id});await refreshData(true);toast(`Booking cancelled. Refund: ${money(result.refund)}.`); }
  catch(error){toast(error.message,'error');}
}
async function joinWaitlist(showId) {
  const customerId=Number($('#waitlist-customer')?.value||0);
  if(!customerId)return toast('Choose a customer first.','error');
  try{const result=await apiPost('waitlist',{customerId,showId});await refreshData(true);toast(result.message);}catch(error){toast(error.message,'error');}
}
function globalSearch(value) {
  const query=value.trim().toLowerCase();
  if(!query){searchPopover?.remove();searchPopover=null;return;}
  const results=[];
  appState.movies.filter(x=>x.title.toLowerCase().includes(query)).slice(0,3).forEach(x=>results.push({label:x.title,meta:'Movie',page:'movies'}));
  appState.customers.filter(x=>x.name.toLowerCase().includes(query)||x.phone.includes(query)).slice(0,3).forEach(x=>results.push({label:x.name,meta:`Customer · ${x.phone}`,page:'customers'}));
  appState.bookings.filter(x=>x.id.toLowerCase().includes(query)).slice(0,3).forEach(x=>results.push({label:x.id,meta:`Booking · ${x.movieTitle}`,page:'booking'}));
  if(!searchPopover){searchPopover=document.createElement('div');searchPopover.className='search-popover';$('.global-search').appendChild(searchPopover);}
  searchPopover.innerHTML=results.length?results.map(r=>`<button data-action="search-result" data-page="${r.page}"><strong>${esc(r.label)}</strong><small>${esc(r.meta)}</small></button>`).join(''):'<div class="search-empty">No matching records</div>';
}

document.addEventListener('click',async event=>{
  const nav=event.target.closest('[data-page]');
  if(nav){setPage(nav.dataset.page);return;}
  const action=event.target.closest('[data-action]');
  if(!action)return;
  if(action.dataset.action==='seat'){
    const row=Number(action.dataset.row),col=Number(action.dataset.col);
    selectedSeat=selectedSeat&&selectedSeat.row===row&&selectedSeat.col===col?null:{row,col};
    const show=appState.shows.find(s=>s.id===selectedShowId);if(show)renderSeatMap(show);renderBookingSummary(show);return;
  }
  if(action.dataset.action==='book'){await submitBooking();return;}
  if(action.dataset.action==='cancel'){await cancelBooking(action.dataset.id);return;}
  if(action.dataset.action==='waitlist'){await joinWaitlist(Number(action.dataset.show));return;}
  if(action.dataset.action==='search-result'){setPage(action.dataset.page);searchPopover?.remove();searchPopover=null;$('#global-search').value='';return;}
});
$('#global-search').addEventListener('input',event=>globalSearch(event.target.value));
document.addEventListener('keydown',event=>{if((event.metaKey||event.ctrlKey)&&event.key.toLowerCase()==='k'){event.preventDefault();$('#global-search').focus();}if(event.key==='Escape')closeModal();});

(async function boot(){
  try {
    appState=await apiGet('/api/state');
    renderSidebarDate();
    renderCurrentPage();
    setTimeout(()=>$('#loading-screen').classList.add('done'),450);
  } catch(error) {
    $('#loading-screen').innerHTML=`<div class="loading-brand"><strong>CINÉ<span>.</span></strong></div><p>${esc(error.message)}</p><p>Close this page, restart start_localhost.bat, then refresh.</p>`;
  }
})();
