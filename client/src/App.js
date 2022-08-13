import { Container } from "react-bootstrap";
import { ServerStatus } from "./actions/ServerStatus";
import Header from "./layout/Header";

function App() {
  return (
    <Container className="p-3">
      <Header />
      <Container className="p-5 mb-4 bg-light rounded-3">
        <h1 className="header">Welcome To Beat Server</h1>
      </Container>
      <ServerStatus />
    </Container>
  );
}

export default App;
