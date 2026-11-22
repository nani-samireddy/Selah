/* Bundled, offline LAN client. Never queues commands or stores pairing credentials. */
'use strict';
const $ = id => document.getElementById(id);
let live = null, etag = '', deckSnapshot = null, previewKey = '', online = false;
let busy = false, paired = false, failures = 0, timer = null, generation = 0;
let clockOffset = 0, deckGeneration = 0;
history.scrollRestoration = 'manual';
const fragment = new URLSearchParams(location.hash.slice(1));
const pairingFromQr = fragment.has('control') || fragment.has('speaker');
$('code').value = fragment.get('control') || fragment.get('speaker') || '';
history.replaceState(null, '', location.pathname);

async function request(path, data, headers = {}) {
  const abort = new AbortController();
  const timeout = setTimeout(() => abort.abort(), 3000);
  try {
    const response = await fetch(path, {
      method: data === undefined ? 'GET' : 'POST', credentials: 'same-origin',
      cache: 'no-store', signal: abort.signal,
      headers: data === undefined ? headers : { 'Content-Type': 'application/json' },
      body: data === undefined ? undefined : JSON.stringify(data)
    });
    if (response.status === 304) return { unchanged: true };
    const value = await response.json();
    if (!response.ok) throw Object.assign(new Error(value.error || 'Request failed.'), { status: response.status });
    return { value, etag: response.headers.get('ETag') };
  } finally { clearTimeout(timeout); }
}
function connection(connected, message) {
  online = connected;
  $('connection').textContent = message;
  $('connection').classList.toggle('online', connected);
  controls();
}
function controls() {
  const allowed = online && !busy && live?.role === 'controller' && live.index >= 0;
  $('previous').disabled = !allowed || live.index <= 0;
  $('next').disabled = !allowed || live.index >= live.count - 1;
  $('blackout').disabled = !allowed;
  document.querySelectorAll('[data-jump]').forEach(button => { button.disabled = !allowed; });
}
function unpair(message = '') {
  paired = false; ++generation; ++deckGeneration;
  clearTimeout(timer); live = null; etag = ''; deckSnapshot = null; previewKey = '';
  $('remote').hidden = true; $('pairing').hidden = false;
  $('currentPreview').removeAttribute('src'); $('nextPreview').removeAttribute('src');
  $('speakerText').textContent = ''; $('notes').textContent = ''; $('items').replaceChildren();
  $('pairError').textContent = message; connection(false, 'Not paired');
}
function render(state) {
  if (live?.role !== state.role) previewKey = '';
  live = state;
  if (state.hostTime) clockOffset = state.hostTime - Date.now();
  const controller = state.role === 'controller';
  $('controllerView').hidden = !controller; $('speakerView').hidden = controller;
  $('role').textContent = controller ? 'REMOTE CONTROLLER' : 'SPEAKER VIEW · READ ONLY';
  $('position').textContent = state.count ? `Slide ${state.index + 1} of ${state.count}` : 'Nothing live';
  $('outputStatus').textContent = [state.audienceEnabled ? 'Audience output on' : 'Audience output stopped', state.blackout ? 'BLACKOUT' : '', state.defaultImageShowing ? 'Default image showing' : '', state.textCleared ? 'Text cleared' : ''].filter(Boolean).join(' · ');
  $('currentTitle').textContent = state.title; $('nextTitle').textContent = state.nextTitle || 'End of presentation';
  $('blackout').textContent = state.blackout ? 'Restore audience' : 'Blackout';
  $('blackout').classList.toggle('active', state.blackout);
  $('emptyLive').hidden = state.count > 0;
  const key = `${state.snapshot}:${state.index}`;
  if (previewKey !== key) {
    previewKey = key;
    if (controller && state.index >= 0) $('currentPreview').src = '/api/preview/current';
    else $('currentPreview').removeAttribute('src');
    if (controller && state.index + 1 < state.count) $('nextPreview').src = '/api/preview/next';
    else $('nextPreview').removeAttribute('src');
    $('nextPreview').hidden = state.index + 1 >= state.count;
    $('currentPreview').hidden = state.index < 0;
  }
  for (const [id, keyName] of Object.entries({ speakerTitle: 'title', speakerText: 'text', speakerTranslation: 'translation', speakerNextTitle: 'nextTitle', speakerNextText: 'nextText', speakerNextTranslation: 'nextTranslation', countdown: 'countdown' })) $(''+id).textContent = state[keyName] || '';
  if (state.count === 0) $('speakerText').textContent = 'Waiting for a live presentation';
  $('notes').textContent = state.notes || 'No notes for this slide.';
  $('speakerMessage').textContent = state.message; $('speakerMessage').hidden = !state.message;
  document.querySelectorAll('.slidejump').forEach(button => {
    const current = Number(button.dataset.jump) === state.index;
    button.classList.toggle('current', current);
    if (current) button.setAttribute('aria-current', 'step'); else button.removeAttribute('aria-current');
  });
  controls();
  if (deckSnapshot !== state.snapshot) loadDeck(state.snapshot);
}
async function loadDeck(snapshot) {
  deckSnapshot = snapshot;
  const ticket = ++deckGeneration;
  try {
    const { value: deck } = await request('/api/deck');
    if (ticket !== deckGeneration || !paired || live?.snapshot !== deck.snapshot) return;
    $('programTitle').textContent = deck.title;
    const fragment = document.createDocumentFragment();
    for (const item of deck.items) {
      const section = document.createElement('section'); section.className = 'serviceitem';
      const title = document.createElement('div'); title.className = 'itemtitle';
      const button = document.createElement('button'); button.textContent = item.title;
      button.dataset.jump = item.start;
      button.addEventListener('click', () => command('jump', { index: item.start }));
      const count = document.createElement('span'); count.textContent = `${item.slides.length} slides`;
      title.append(button, count); section.append(title);
      const slides = document.createElement('div'); slides.className = 'slides';
      for (const page of item.slides) {
        const jump = document.createElement('button'); jump.className = 'slidejump';
        jump.dataset.jump = page.index; jump.textContent = String(page.index + 1); jump.title = page.title;
        jump.setAttribute('aria-label', `Slide ${page.index + 1}: ${page.title}`);
        jump.addEventListener('click', () => command('jump', { index: page.index })); slides.append(jump);
      }
      section.append(slides); fragment.append(section);
    }
    $('items').replaceChildren(fragment); render(live);
  } catch (error) {
    if (ticket === deckGeneration) deckSnapshot = null;
  }
}
async function poll(ticket = generation) {
  if (!paired || ticket !== generation) return;
  try {
    const response = await request('/api/state', undefined, etag ? { 'If-None-Match': etag } : {});
    if (ticket !== generation) return;
    if (!response.unchanged) { etag = response.etag || ''; render(response.value); }
    failures = 0; connection(true, 'Connected');
    if ($('notice').dataset.offline) { $('notice').textContent = ''; delete $('notice').dataset.offline; }
  } catch (error) {
    if (ticket !== generation) return;
    if (error.status === 401) { unpair('Your session ended. Enter the current pairing code.'); return; }
    ++failures; connection(false, 'Reconnecting…');
    $('notice').textContent = 'Connection lost. Controls are paused while reconnecting.';
    $('notice').dataset.offline = 'true';
  }
  if (paired && ticket === generation) timer = setTimeout(() => poll(ticket), Math.min(5000, 1000 * Math.max(1, failures)));
}
async function command(action, extra = {}) {
  if (!online || busy || live?.role !== 'controller') return;
  busy = true; controls();
  // Fresh randomness works on plain LAN HTTP, where crypto.randomUUID may be unavailable.
  const bytes = crypto.getRandomValues(new Uint8Array(16));
  const requestId = Array.from(bytes, byte => byte.toString(16).padStart(2, '0')).join('');
  const ticket = generation;
  // Supersede an in-flight poll so an older response cannot undo this command's state.
  clearTimeout(timer); ++generation;
  try {
    const { value } = await request('/api/command', { action, requestId, snapshot: live.snapshot, expectedIndex: live.index, ...extra });
    if (!paired) return;
    etag = ''; render(value); $('notice').textContent = '';
  } catch (error) {
    if (error.status === 401) unpair('Your session ended. Pair again to continue.');
    else {
      $('notice').textContent = error.status === 409 ? error.message : 'Command was not confirmed. Check the live state before trying again.';
      etag = ''; connection(false, 'Refreshing…');
    }
  } finally {
    busy = false; controls();
    if (paired && ticket + 1 === generation) poll(generation);
  }
}
$('pairForm').addEventListener('submit', async event => {
  event.preventDefault(); $('connect').disabled = true; $('pairError').textContent = '';
  try {
    await request('/api/pair', { code: $('code').value.trim(), name: $('name').value });
    $('code').value = ''; $('pairing').hidden = true; $('remote').hidden = false;
    paired = true; etag = ''; ++generation; window.scrollTo(0, 0); poll(generation);
  } catch (error) { $('pairError').textContent = error.status ? error.message : 'Cannot reach the presentation computer.'; }
  finally { $('connect').disabled = false; }
});
$('previous').addEventListener('click', () => command('previous'));
$('next').addEventListener('click', () => command('next'));
$('blackout').addEventListener('click', () => command('blackout', { value: !live.blackout }));
$('disconnect').addEventListener('click', async () => {
  try { await request('/api/logout', {}); unpair(); }
  catch { $('notice').textContent = 'Could not disconnect. Retry when connected, or revoke this device from the desktop.'; }
});
setInterval(() => { $('clock').textContent = new Date(Date.now() + clockOffset).toLocaleTimeString([], { hour: '2-digit', minute: '2-digit', second: '2-digit' }); }, 1000);
// Reloads reuse the HttpOnly session cookie; revoked/expired cookies return to pairing.
(async () => {
  // A newly scanned role-specific QR must ask to pair even if this browser has a session.
  if (pairingFromQr) return;
  try {
    const response = await request('/api/state'); paired = true;
    $('pairing').hidden = true; $('remote').hidden = false; window.scrollTo(0, 0); render(response.value);
    connection(true, 'Connected'); poll(generation);
  } catch { /* An unpaired browser stays on the pairing screen. */ }
})();
