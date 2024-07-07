import { useLoaderData, useFetcher, useSubmit } from "react-router-dom";
import PanelHeader from "../components/PanelHeader";
import {
  Row,
  Col,
  Card,
  CardHeader,
  CardTitle,
  CardBody,
  CardFooter,
  Input,
  FormGroup,
  Label,
  Form,
} from "reactstrap";
import { getAPIHost, setAPIHost } from "../lib/api";

export async function loader({ request }) {
  return { host: getAPIHost() };
}

export async function action({ request, params }) {
  const formData = await request.formData();
  const updates = Object.fromEntries(formData);
  await setAPIHost(updates.host);
  return true;
}

const API_HOSTS = [
  { name: "Localhost", value: "https://localhost:8080" },
  { name: "Raspberry Pi", value: "https://raspberrypi1.local:8080" },
];

export default function ConfigPage() {
  const fetcher = useFetcher();
  const submit = useSubmit();

  const { host } = useLoaderData();

  const onRadioChange = (event) => {
    submit(event.currentTarget.form);
  };

  return (
    <>
      <PanelHeader
        content={
          <div className="header text-center">
            <h2 className="title">Config</h2>
          </div>
        }
      />
      <div className="content">
        <Row>
          <Col xs={12} md={6}>
            <Form method="post">
              <Card className="card-chart">
                <CardHeader>
                  <CardTitle tag="h4">Server config</CardTitle>
                </CardHeader>
                <CardBody></CardBody>
                <CardFooter>
                  <FormGroup tag="fieldset">
                    <legend>API Host</legend>
                    {API_HOSTS.map((api_host) => (
                      <FormGroup check key={api_host.value}>
                        <Input
                          name="host"
                          type="radio"
                          value={api_host.value}
                          checked={host === api_host.value}
                          onChange={onRadioChange}
                        />
                        <Label check>{api_host.name}</Label>
                      </FormGroup>
                    ))}
                  </FormGroup>
                </CardFooter>
              </Card>
            </Form>
          </Col>
        </Row>
      </div>
    </>
  );
}
