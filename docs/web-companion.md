# Web Companion

Static files at the repo root. They are not part of the VPK.

## Verification page — `index.html`, `script.js`, `style.css`

The page the Vita browser opens during the OAuth device flow. `displayUserCode()` in
`gdrive.cpp` opens:

```
https://add-le.github.io/ps-vita-sync/?user_code=<code>&expires_in=<seconds>&verification_url=<url>
```

So it is served by **GitHub Pages from this repository**. Changing these files changes the
live page once Pages publishes them.

Behavior (`script.js`, `init()` on load):

1. **Device gate:** if the user agent does not contain `PlayStation Vita`, or the window/screen
   is not 960×544, the body is replaced with an `<object>` showing `error.html`
   ("only accessible with a PlayStation Vita device").
2. Parses `user_code`, `expires_in`, `verification_url` from `location.search` by splitting
   on `&` and `=`. No `decodeURIComponent`.
3. Countdown timer: the first visit stores `{ts, expires_in}` in `localStorage` keyed by
   `user_code`, so reloading continues the countdown rather than restarting it. At zero it
   shows "The code has expired, make a new request in the application."
4. Writes the code and URL with `innerHTML` and sets the link `href` to `verification_url`
   (CODE_REVIEW Low: reflected XSS / `javascript:` URL; use `textContent` and validate the
   scheme).

The user is expected to read the code on the Vita, then open `verification_url`
(`https://www.google.com/device`) on the Vita or another device and enter it.

Constraints: the Vita browser is an old WebKit (the file header says "JavaScriptCore
Engine 2009"), so `script.js` uses ES5 only (`var`, `function`, no arrow functions, no
template literals). Keep it that way. Styling uses Bulma 0.9.4 from jsDelivr plus
`style.css`.

## Other files

| File | What it is |
|---|---|
| `acces_token.min.html` | Single-file minified variant of the verification page (inline CSS/JS). Not referenced by the app. |
| `error.html` | "Wrong device" page shown inside `index.html` for non-Vita browsers. |
| `file_explorer.html/.css/.js` | Browser prototype of the touch file explorer, including a `Path` class with an `isSync` flag that propagates to children. Design reference for `gui.cpp`. |
| `verification_page.jpg` | Image of the verification page design. |
| `ps_vita_sync_logo_*`, `.ico` | Logos used by the pages. |

`.prettierignore` excludes `tsconfig.json`; HTML/CSS/JS are presumably formatted with
Prettier defaults plus `.editorconfig`.