import { VolumeUp } from "react-bootstrap-icons";

function Header() {
  return (
    <header class="d-flex flex-wrap justify-content-center py-3 mb-4 border-bottom">
      <a
        href="/"
        class="d-flex align-items-center mb-3 mb-md-0 me-md-auto text-dark text-decoration-none"
      >
        <span class="fs-4">
          <VolumeUp />
          Beat Server
        </span>
      </a>

      {/* <ul class="nav nav-pills">
        <li class="nav-item">
          <a href="#" class="nav-link active" aria-current="page">
            Home
          </a>
        </li>
        <li class="nav-item">
          <a href="#" class="nav-link">
            About
          </a>
        </li>
      </ul> */}
    </header>
  );
}

export default Header;
