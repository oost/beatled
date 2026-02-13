import { useLoaderData, useSubmit } from "react-router-dom";
import PageHeader from "../components/page-header";
import { Card, CardContent } from "@/components/ui/card";
import { RadioGroup, RadioGroupItem } from "@/components/ui/radio-group";
import { Label } from "@/components/ui/label";
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
  const submit = useSubmit();
  const { host } = useLoaderData();

  const onValueChange = (value) => {
    const formData = new FormData();
    formData.set("host", value);
    submit(formData, { method: "post" });
  };

  return (
    <>
      <PageHeader title="Config" />
      <div className="px-4 pb-8 md:px-8">
        <Card>
          <CardContent className="pt-6">
            <fieldset>
              <legend className="mb-3 text-sm font-medium">API Host</legend>
              <RadioGroup value={host} onValueChange={onValueChange}>
                {API_HOSTS.map((api_host) => (
                  <div
                    key={api_host.value}
                    className="flex items-center space-x-3 py-2"
                  >
                    <RadioGroupItem
                      value={api_host.value}
                      id={`host-${api_host.value}`}
                    />
                    <Label
                      htmlFor={`host-${api_host.value}`}
                      className="text-sm font-normal"
                    >
                      {api_host.name}
                    </Label>
                  </div>
                ))}
              </RadioGroup>
            </fieldset>
          </CardContent>
        </Card>
      </div>
    </>
  );
}
