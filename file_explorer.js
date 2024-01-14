class Path {
  filename;
  children;
  parent;

  constructor(filename, children) {
    this.filename = filename;
    if (children) {
      this.appendChild(children);
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

function getFolderIcon() {
  return getMaterialSymbols("folder");
}

function getMaterialSymbols(name) {
  const span = document.createElement("span");
  span.classList.add("material-symbols-outlined");
  span.innerText = name;
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

function getCheckbox() {
  const checkbox = document.createElement("input");
  checkbox.type = "checkbox";
  checkbox.classList.add("checkbox");
  return checkbox;
}

function getPathItem(name, isEmpty) {
  const div = document.createElement("div");
  div.classList.add("selected-item");
  div.innerText = name;
  if (isEmpty) div.id = "is-empty";
  return div;
}

let selected = 0;
function displayPath(_path) {
  const container = document.getElementById("container");
  container.innerHTML = null;

  const info = document.getElementById("info");
  const __path = document.getElementById("path");
  const header = document.getElementById("header");
  header.innerText = `${selected} selected`;

  console.log("logger", _path);

  if (_path.parent) {
    container.appendChild(getBackButton(_path));
  }

  const root_path = ls(_path);
  root_path.forEach((path) => {
    const div = document.createElement("div");
    const checkbox = getCheckbox();
    div.appendChild(checkbox);
    div.classList.add("path-tile");
    checkbox.onclick = (ev) => {
      ev.stopPropagation();
      const isAdded = div.classList.toggle("selected");
      selected += isAdded ? 1 : -1;
      if (selected > 0) {
        header.style.visibility = "visible";
        header.style.opacity = 1;
        header.innerText = `${selected} selected`;
      } else {
        header.style.visibility = "hidden";
        header.style.opacity = 0;
      }
      info.innerText = `Files: ${root_path.length}`;
    };

    const span = document.createElement("span");
    span.classList.add("center");

    // Is folder
    if (Array.isArray(path.children)) {
      span.appendChild(getFolderIcon());
      div.classList.add("cursor");
      div.onclick = () => {
        header.style.visibility = "hidden";
        header.style.opacity = 0;
        selected = 0;
        displayPath(path);
      };
    }

    span.appendChild(document.createTextNode(path.filename));

    div.appendChild(span);
    container.appendChild(div);
  });

  __path.appendChild(getPathItem(_path.filename));
  const selectedItems = document.getElementsByClassName("selected-item");
  if (selectedItems.length % 2 == 0) {
    __path.appendChild(getPathItem("", true));
  } else {
    const spacer = document.getElementById("is-empty");
    if (spacer) {
      spacer.remove();
    }
  }

  info.innerText = `Files: ${root_path.length}`;
}

function init() {
  const root = new Path("/", [
    new Path("Audio", [new Path("horse.mp3")]),
    new Path("Demo", []),
    new Path("Images", [
      new Path("Screenshots", [new Path("Diablo VI", []), new Path("pgm.jpg")]),
      new Path("region.jpg"),
      new Path("france.jpg"),
    ]),
    new Path("Video", [new Path("journey.mp4")]),
    new Path("config.php"),
    new Path("style.css"),
    new Path("index.php"),
    new Path("translation.json"),
    new Path("repos", [new Path("sqlite")]),
  ]);

  displayPath(root);
}
