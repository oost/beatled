import PanelHeader from "../components/PanelHeader";
import {
  Row,
  Col,
  Card,
  CardHeader,
  CardTitle,
  UncontrolledDropdown,
  DropdownToggle,
  DropdownMenu,
  DropdownItem,
  CardBody,
  CardFooter,
  Table,
  FormGroup,
  Label,
  Input,
  Button,
  UncontrolledTooltip,
} from "reactstrap";
import { Line, Bar } from "react-chartjs-2";
import { FormattedNumber } from "react-intl";

import {
  Chart as ChartJS,
  CategoryScale,
  LinearScale,
  PointElement,
  LineElement,
  Tooltip,
  Legend,
} from "chart.js";
import moment from "moment";
import BeatChart from "../components/status/BeatChart";

ChartJS.register(
  CategoryScale,
  LinearScale,
  PointElement,
  LineElement,
  Tooltip,
  Legend
);
import { useLoaderData, Form, useFetcher } from "react-router-dom";
import { useState } from "react";
import { useInterval } from "../hooks/interval";

const props = [
  { label: "Status", key: "status", format: (v) => v },
  {
    label: "Tempo",
    key: "tempo",
    format: (v) => <FormattedNumber value={v} maximumFractionDigits={1} />,
  },
  {
    label: "Last update",
    key: "updateTime",
    format: (v) => v.format("h:mm:ss a"),
  },
];

const tempoHistory = [];
const MAX_HISTORY = 30;

export async function loader({ request }) {
  console.log("Loading status");
  const tempo = 120 + 40 * 2 * (Math.random() - 0.5);
  tempoHistory.push({ x: new Date(), y: tempo });
  if (tempoHistory.length > MAX_HISTORY) {
    tempoHistory.slice(tempoHistory.length - MAX_HISTORY);
  }
  return {
    last: {
      status: "Running",
      tempo,
      updateTime: new Date(),
    },
    history: tempoHistory,
  };
}

export async function action({ request, params }) {
  // let formData = await request.formData();
  return true;
}

export default function Status() {
  const fetcher = useFetcher();
  const statusData = useLoaderData();

  useInterval(() => {
    if (fetcher.state === "idle") {
      fetcher.submit();
    }
  }, 2 * 1000);

  const data = fetcher.data.last ? fetcher.data : statusData.last;
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
          <Col xs={12} md={4}>
            <Card className="card-chart">
              <CardHeader>
                {/* <h5 className="card-category">Status</h5> */}
                <CardTitle tag="h4">Beatled Status</CardTitle>
                <fetcher.Form method="post">
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
                      <i className="now-ui-icons arrows-1_refresh-69" />
                    </button>
                  </div>
                </fetcher.Form>
              </CardHeader>
              <CardBody>
                {" "}
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
          </Col>
        </Row>
        <Row>
          <Col xs={12} md={4}>
            <BeatChart />
          </Col>
        </Row>
      </div>
    </>
  );
}
