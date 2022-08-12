import {Container} from 'react-bootstrap';
import { ServerStatus } from './server-status';

function App() {
  return (
    <Container className="p-3">
    <Container className="p-5 mb-4 bg-light rounded-3">
      <h1 className="header">Welcome To React-Bootstrap</h1>
      <ServerStatus />

    </Container>
  </Container>
  );
}

export default App;
