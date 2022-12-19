import {
  Card,
  CardHeader,
  CardTitle,
  UncontrolledDropdown,
  DropdownToggle,
  DropdownMenu,
  DropdownItem,
  CardBody,
  CardFooter,
} from "reactstrap";
import { Line } from "react-chartjs-2";
import {
  Chart as ChartJS,
  CategoryScale,
  LinearScale,
  PointElement,
  LineElement,
  Tooltip,
  Legend,
  TimeScale,
} from "chart.js";
import "chartjs-adapter-date-fns";
import { enUS } from "date-fns/locale";

ChartJS.register(
  CategoryScale,
  LinearScale,
  PointElement,
  LineElement,
  Tooltip,
  Legend,
  TimeScale
);

function getChartConfig(historyData) {
  return {
    data: {
      datasets: [
        {
          label: "Tempo",
          borderColor: "#f96332",
          pointBorderColor: "#FFF",
          pointBackgroundColor: "#f96332",
          pointBorderWidth: 2,
          pointHoverRadius: 4,
          pointHoverBorderWidth: 1,
          pointRadius: 4,
          // fill: true,
          // backgroundColor: gradientFill,
          borderWidth: 2,
          tension: 0.4,
          data: historyData,
        },
      ],
    },
    options: {
      scales: {
        x: {
          type: "time",
          adapters: {
            date: {
              locale: enUS,
            },
          },
        },
      },
    },
  };
}

export default function BeatChart({ historyData }) {
  const chartConfig = getChartConfig(historyData);
  return (
    <>
      <Card className="card-chart">
        <CardHeader>
          <CardTitle tag="h4">Beat History</CardTitle>
        </CardHeader>
        <CardBody>
          <div className="chart-area">
            <Line data={chartConfig.data} options={chartConfig.options} />
          </div>
        </CardBody>
        <CardFooter>
          <div className="stats">
            <i className="now-ui-icons arrows-1_refresh-69" /> Just Updated
          </div>
        </CardFooter>
      </Card>
    </>
  );
}
