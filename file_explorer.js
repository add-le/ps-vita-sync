class Path {
  filename;
  children;
  parent;
  isSync;

  constructor(filename, children, isSync) {
    this.filename = filename;
    if (children) {
      this.appendChild(children);
      children.forEach((child) => {
        child.isSync = isSync || child.isSync;
      });
    }
    this.setWholePathToSync(isSync);
  }

  setWholePathToSync(isSync) {
    this.isSync = isSync || this.isSync;
    if (this.children) {
      this.children.forEach((child) => {
        child.setWholePathToSync(isSync);
      });
    }
  }

  appendChild(children) {
    this.children = children;
    this.children.forEach((child) => {
      child.parent = this;
    });
  }
}

function ls(path) {
  return path.children;
}

function getIcon(filename, isFolder) {
  if (isFolder) return getMaterialSymbols("folder");
  if (
    [
      ".png",
      ".jpg",
      ".jpeg",
      ".bmp",
      ".gif",
      ".webp",
      ".svg",
      ".apng",
      ".avif",
      ".ico",
      ".tiff",
      ".tif",
    ].some((ext) => filename.toLowerCase().endsWith(ext))
  )
    return getMaterialSymbols("image");
  if (
    [".mp3", ".ogg", ".wav", ".flac", ".m4a", ".wma", ".webm"].some((ext) =>
      filename.toLowerCase().endsWith(ext)
    )
  )
    return getMaterialSymbols("headphones");
  if ([".php"].some((ext) => filename.toLowerCase().endsWith(ext)))
    return getMaterialSymbols("php");

  if ([".css"].some((ext) => filename.toLowerCase().endsWith(ext)))
    return getMaterialSymbols("css");

  if ([".js"].some((ext) => filename.toLowerCase().endsWith(ext)))
    return getMaterialSymbols("javascript");

  if ([".html"].some((ext) => filename.toLowerCase().endsWith(ext)))
    return getMaterialSymbols("html");

  if ([".json"].some((ext) => filename.toLowerCase().endsWith(ext)))
    return getMaterialSymbols("data_object");

  return getMaterialSymbols("note");
}

function getMaterialSymbols(name) {
  const span = document.createElement("span");
  span.classList.add("material-symbols-outlined");
  span.innerText = name;
  span.dataset.old_icon = name;
  return span;
}

function getBackButton(_path) {
  const div = document.createElement("div");
  div.classList.add("path-tile", "cursor");
  div.style.justifyContent = "center";
  div.appendChild(getMaterialSymbols("arrow_back"));
  div.onclick = () => {
    const header = document.getElementById("header");
    header.style.visibility = "hidden";
    header.style.opacity = 0;
    selected = 0;
    displayPath(_path.parent);
  };
  return div;
}

function selectAll() {
  executeSelect(true);
}

function deselectAll() {
  executeSelect(false);
}

function executeSelect(state) {
  const tiles = document.querySelectorAll(".path-tile");
  const header = document.getElementById("header");
  const headerText = document.getElementById("header-text");

  for (const tile of tiles) {
    toggleSelected(
      { stopPropagation: () => {} },
      tile,
      header,
      {},
      { length: "" },
      headerText,
      state
    );
  }
}

function cloudSync() {
  const tiles = document.querySelectorAll(".path-tile.selected");
  const names = [];
  for (const tile of tiles) names.push(tile.innerText.split("\n")[1]);
  console.log("selected item(s): ", names);
}

let selected = 0;
let timer;

function toggleSelected(ev, div, header, info, root_path, headerText, state) {
  ev.stopPropagation();
  const isAdded = div.classList.toggle("selected", state);
  const span_icon = div.querySelector("span");
  if (isAdded) {
    if (!span_icon.dataset.old_icon) {
      span_icon.dataset.old_icon = span_icon.innerText;
    }
    span_icon.innerText = "check_circle";
  } else {
    span_icon.innerText = span_icon.dataset.old_icon;
  }

  selected = document.querySelectorAll(".path-tile.selected").length;
  if (selected > 0) {
    header.style.visibility = "visible";
    header.style.opacity = 1;
    headerText.innerText = `${selected} item${selected > 1 ? "s" : ""}`;
  } else {
    header.style.visibility = "hidden";
    header.style.opacity = 0;
  }
}

function displayPath(_path) {
  const container = document.getElementById("container");
  container.innerHTML = null;

  const header = document.getElementById("header");
  const headerText = document.getElementById("header-text");
  headerText.innerText = `${selected} selected`;

  if (_path.parent) {
    container.appendChild(getBackButton(_path));
  }

  const root_path = ls(_path);
  root_path.forEach((path) => {
    const div = document.createElement("div");
    div.classList.add("path-tile");
    const span = getIcon(path.filename, Array.isArray(path.children));
    span.classList.add("icon");

    // Is is sync add sync icon
    if (path.isSync) {
      const sync = getMaterialSymbols("download_done");
      sync.classList.add("sync-bubble", "is-sync-icon");
      span.appendChild(sync);
    }

    div.appendChild(span);
    // Is folder
    div.classList.add("cursor");
    div.onclick = (ev) => {
      if (selected != 0) {
        toggleSelected(ev, div, header, {}, root_path, headerText);
      } else if (Array.isArray(path.children)) {
        header.style.visibility = "hidden";
        header.style.opacity = 0;
        selected = 0;
        displayPath(path);
      }
    };
    div.addEventListener(
      "touchstart",
      (ev) => {
        timer = setTimeout(() => {
          toggleSelected(ev, div, header, {}, root_path, headerText);
        }, 500);
      },
      { passive: true }
    );
    div.ontouchend = () => {
      clearTimeout(timer);
    };

    const spanText = document.createElement("span");
    spanText.classList.add("path-tile-text");
    spanText.innerText = path.filename;

    div.appendChild(spanText);
    container.appendChild(div);
  });
}

function init() {
  const root = new Path("/", [
    new Path("app", [new Path("horse.mp3")], true),
    new Path("data", []),
    new Path(
      "Images",
      [
        new Path("Screenshots", [
          new Path("Diablo VI", []),
          new Path("pgm.jpg"),
        ]),
        new Path("region.PNG"),
        new Path("france.jpg"),
      ],
      true
    ),
    new Path("Video", [new Path("journey.mp4")], false),
    new Path("config.php"),
    new Path("style.css"),
    new Path("index.php"),
    new Path("translation.json"),
    new Path("repos", [new Path("sqlite", null, true)], false),
    new Path(
      "This is a name file very long very very long name file in fact it is too long name file.txt"
    ),
    new Path("WWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWW"),
  ]);

  displayPath(root);
}
