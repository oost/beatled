import { useLoaderData, Form, useFetcher } from "react-router-dom";
import { useInterval } from "../hooks/interval";
import { getStatus } from "../lib/status";
import PanelHeader from "../components/PanelHeader";
import {
  Row,
  Col,
  Card,
  CardHeader,
  CardTitle,
  CardBody,
  CardFooter,
  Table,
  Input,
  FormGroup,
  Label,
} from "reactstrap";
import { FormattedNumber } from "react-intl";

import BeatChart from "../components/status/BeatChart";
import { format } from "date-fns";
import { ArrowClockwise } from "react-bootstrap-icons";

// import BeatChart from "../components/status/BeatChart";

const props = [
  { label: "Status", key: "status", format: (v) => v },
  {
    label: "Tempo",
    key: "tempo",
    format: (v) => (
      <FormattedNumber
        value={v}
        minimumFractionDigits={1}
        maximumFractionDigits={2}
      />
    ),
  },
  {
    label: "Last update",
    key: "x",
    format: (v) => format(v, "h:mm:ss a"),
  },
  {
    label: "Beat Detector",
    key: "beat_detector",
    format: (v) => (
      <FormGroup switch disabled>
        <Input type="switch" checked={v} onChange={() => {}} />
        <Label check>Default switch checkbox input</Label>
      </FormGroup>
    ),
  },
  {
    label: "Broadcaster",
    key: "broadcaster",
    format: (v) => (v ? "On" : "Off"),
  },
  {
    label: "UDP Server",
    key: "udp_server",
    format: (v) => (v ? "On" : "Off"),
  },
  {
    label: "HTTP Server",
    key: "http_server",
    format: (v) => (v ? "On" : "Off"),
  },
  {
    label: "Message",
    key: "message",
    format: (v) => v,
  },
];

const tempoHistory = [];
const MAX_HISTORY = 30;

export async function loader({ request }) {
  console.log("Loading status");
  const status = await getStatus();
  // const tempo = 120 + 40 * 2 * (Math.random() - 0.5);
  const last = { x: new Date(), y: status.tempo || NaN, ...status };
  tempoHistory.push(last);
  if (tempoHistory.length > MAX_HISTORY) {
    tempoHistory.splice(0, tempoHistory.length - MAX_HISTORY);
  }
  return {
    last,
    history: tempoHistory,
  };
}

export async function action({ request, params }) {
  return true;
}

export default function StatusPage() {
  const fetcher = useFetcher();
  const statusData = useLoaderData();

  useInterval(() => {
    if (fetcher.state === "idle") {
      fetcher.submit();
    }
  }, 2 * 1000);

  const data = fetcher.data ? fetcher.data.last : statusData.last;
  const historyData = fetcher.data ? fetcher.data.history : statusData.history;
  return (
    <>
      <PanelHeader
        content={
          <div className="header text-center">
            <h2 className="title">Status</h2>
          </div>
        }
      />
      <div className="content">
        <Row>
          <Col xs={12} md={6}>
            <fetcher.Form method="post">
              <Card className="card-chart">
                <CardHeader>
                  {/* <h5 className="card-category">Status</h5> */}
                  <CardTitle tag="h4">Beatled Status</CardTitle>
                  <div
                    onClick={(event) => {
                      fetcher.submit();
                    }}
                    className="dropdown right"
                  >
                    <button
                      type="button"
                      aria-haspopup="true"
                      aria-expanded="true"
                      className="btn-round btn-outline-default btn-icon btn btn-default"
                    >
                      <ArrowClockwise />
                    </button>
                  </div>
                </CardHeader>
                <CardBody>
                  <Table responsive>
                    <tbody>
                      {props.map((prop) => {
                        return (
                          <tr key={prop.key}>
                            <th>{prop.label}</th>
                            <td>{prop.format(data[prop.key])}</td>
                          </tr>
                        );
                      })}
                    </tbody>
                  </Table>
                </CardBody>
                <CardFooter>
                  <div className="stats"></div>
                </CardFooter>
              </Card>
            </fetcher.Form>
          </Col>

          <Col xs={12} md={6}>
            <Card className="card-chart">
              <CardHeader>
                <CardTitle tag="h4">Beat History</CardTitle>
              </CardHeader>
              <CardBody>
                <BeatChart historyData={historyData} />
              </CardBody>
              <CardFooter>
                <div className="stats">
                  <i className="now-ui-icons arrows-1_refresh-69" /> Just
                  Updated
                </div>
              </CardFooter>
            </Card>
          </Col>
        </Row>
        <FormGroup switch disabled>
          <Input type="switch" checked={true} onChange={() => {}} />
          <Label check>Default switch checkbox input</Label>
        </FormGroup>
        <div className="form-check form-switch">
          <input
            className="form-check-input"
            type="checkbox"
            role="switch"
            id="flexSwitchCheckDefault"
          />
          <label className="form-check-label" htmlFor="flexSwitchCheckDefault">
            Default switch checkbox input
          </label>
        </div>
      </div>
    </>
  );
}
