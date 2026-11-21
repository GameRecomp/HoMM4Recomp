// SPDX-License-Identifier: MIT
// The games the converter makes builds of, as the page, the worker and the player name them. The
// id is the converter's (games/<id>/game.toml) and the directory the browser stores the game in.
// `source` is what the user brings: the GOG installer, or for the Saga (a German CD, never sold by
// GOG) a zip of the installed game.
export const GAMES = {
  cultures1: {
    id: 'cultures1',
    name: 'Cultures',
    full: 'Cultures: Discovery of Vinland',
    stem: 'Cultures',
    installer: 'setup_cultures_2.1.0.12.exe',
    installerSize: 'about 190 MiB',
    size: 'about 270 MiB',
    web: 'cultures1',
    icon: 'cultures1.svg',
  },
  cultures2: {
    id: 'cultures2',
    name: 'Cultures 2',
    full: 'Cultures 2: The Gates of Asgard',
    stem: 'Cultures2',
    installer: 'setup_cultures2_2.1.0.10.exe',
    installerSize: 'about 290 MiB',
    size: 'about 450 MiB',
    web: 'cultures2',
    icon: 'cultures2.svg',
  },
  northland: {
    id: 'northland',
    name: 'Northland',
    full: 'Cultures: Northland',
    stem: 'Northland',
    installer: 'setup_northland_2.1.0.9.exe',
    installerSize: 'about 180 MiB',
    size: 'about 330 MiB',
    web: 'northland',
    icon: 'northland.svg',
  },
  eighthwonder: {
    id: 'eighthwonder',
    name: '8th Wonder',
    full: 'Cultures: 8th Wonder of the World',
    stem: 'EighthWonder',
    installer: 'setup_8th_wonder_2.1.0.9.exe',
    installerSize: 'about 200 MiB',
    size: 'about 370 MiB',
    web: 'eighthwonder',
    icon: 'eighthwonder.svg',
  },
  saga: {
    id: 'saga',
    name: 'Saga',
    full: 'Cultures – Die Saga',
    stem: 'CulturesSaga',
    source: 'zip',
    installer: 'Cultures Saga.zip',
    installerSize: 'about 280 MiB',
    size: 'about 470 MiB',
    web: 'saga',
    icon: 'saga.svg',
  },
};

/* The game's icon on the page: the tab's and every img.game-icon. */
export function showIcon(game) {
  for (const l of document.querySelectorAll('link[rel=icon]')) l.href = game.icon;
  for (const i of document.querySelectorAll('img.game-icon')) i.src = game.icon;
}

/* The game a page is for: ?game=<id> (or #<id>), else the default. */
export function gameOf(location) {
  const p = new URLSearchParams(location.search).get('game') || location.hash.replace(/^#/, '');
  return GAMES[p] || GAMES.cultures1;
}
