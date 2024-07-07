import { useFetcher, useLoaderData, useSubmit } from "react-router-dom";
import {
  Card,
  CardBody,
  CardHeader,
  CardTitle,
  CardFooter,
  Col,
  Row,
  FormGroup,
  Form,
  Input,
  Label,
} from "reactstrap";
import PanelHeader from "../components/PanelHeader";
import { getProgram, postProgram } from "../lib/program";

export async function loader({ request }) {
  console.log("Loading programs");
  const programInfo = await getProgram();

  return programInfo;
}

export async function action({ request, params }) {
  const formData = await request.formData();
  const updates = Object.fromEntries(formData);
  await postProgram(parseInt(updates.programId));
  return true;
}

export default function ProgramPage() {
  const fetcher = useFetcher();
  const submit = useSubmit();

  const { programId, programs } = useLoaderData();

  const onRadioChange = (event) => {
    submit(event.currentTarget.form);
  };

  return (
    <>
      <PanelHeader
        content={
          <div className="header text-center">
            <h2 className="title">Program</h2>
          </div>
        }
      />
      <div className="content">
        <Row>
          <Col xs={12} md={6}>
            <Form method="post">
              <Card className="card-chart">
                <CardHeader>
                  <CardTitle tag="h4">Program</CardTitle>
                </CardHeader>
                <CardBody>Current program:</CardBody>
                <CardFooter>
                  <FormGroup tag="fieldset">
                    <legend>Programs</legend>
                    {programs?.map((program) => (
                      <FormGroup check key={program.id}>
                        <Input
                          name="programId"
                          type="radio"
                          value={program.id}
                          checked={program.id === programId}
                          onChange={onRadioChange}
                        />
                        <Label check>{program.name}</Label>
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
