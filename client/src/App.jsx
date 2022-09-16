import { Container } from "react-bootstrap";
import { ServerStatus } from "./actions/ServerStatus";
import { UpdateProgramControl } from "./actions/UpdateProgramControl";
import { TempoControl } from "./actions/TempoControl";
import Header from "./layout/Header";
import { LogControl } from "./actions/LogControl";

function App() {
  return (
    <Container className="p-3">
      <Header />
      <Container className="p-5 mb-4 bg-light rounded-3">
        <h1 className="header">Welcome To Beat Server</h1>
      </Container>
      <ServerStatus />
      <UpdateProgramControl />
      <TempoControl />
      <LogControl />
    </Container>
  );
}

export default App;
