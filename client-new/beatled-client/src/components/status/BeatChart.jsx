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
} from "chart.js";

ChartJS.register(
  CategoryScale,
  LinearScale,
  PointElement,
  LineElement,
  Tooltip,
  Legend
);

import demoData from "../../demo/variables/charts";

export default function BeatChart() {
  return (
    <>
      <Card className="card-chart">
        <CardHeader>
          <h5 className="card-category">Global Sales</h5>
          <CardTitle tag="h4">Shipped Products</CardTitle>
          <UncontrolledDropdown>
            <DropdownToggle
              className="btn-round btn-outline-default btn-icon"
              color="default"
            >
              <i className="now-ui-icons loader_gear" />
            </DropdownToggle>
            <DropdownMenu end>
              <DropdownItem>Action</DropdownItem>
              <DropdownItem>Another Action</DropdownItem>
              <DropdownItem>Something else here</DropdownItem>
              <DropdownItem className="text-danger">Remove data</DropdownItem>
            </DropdownMenu>
          </UncontrolledDropdown>
        </CardHeader>
        <CardBody>
          <div className="chart-area">
            <Line
              data={demoData.dashboardShippedProductsChart.data}
              options={demoData.dashboardShippedProductsChart.options}
            />
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
