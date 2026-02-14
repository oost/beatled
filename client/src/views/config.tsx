import { useLoaderData, useSubmit } from "react-router-dom";
import type { ActionFunctionArgs } from "react-router-dom";
import PageHeader from "../components/page-header";
import { Card, CardContent } from "@/components/ui/card";
import { RadioGroup, RadioGroupItem } from "@/components/ui/radio-group";
import { Label } from "@/components/ui/label";
import { getAPIHost, setAPIHost, getAPIToken, setAPIToken } from "../lib/api";

export async function loader() {
  return { host: getAPIHost(), token: getAPIToken() };
}

export async function action({ request }: ActionFunctionArgs) {
  const formData = await request.formData();
  const updates = Object.fromEntries(formData);
  if (updates.host) {
    setAPIHost(updates.host as string);
  }
  if (formData.has("token")) {
    setAPIToken(updates.token as string);
  }
  return true;
}

const API_HOSTS = [
  { name: "Localhost", value: "https://localhost:8443" },
  { name: "Raspberry Pi", value: "https://raspberrypi1.local:8443" },
];

export default function ConfigPage() {
  const submit = useSubmit();
  const { host, token } = useLoaderData() as { host: string; token: string };

  const onHostChange = (value: string) => {
    const formData = new FormData();
    formData.set("host", value);
    submit(formData, { method: "post" });
  };

  const onTokenBlur = (e: React.FocusEvent<HTMLInputElement>) => {
    const formData = new FormData();
    formData.set("token", e.target.value);
    submit(formData, { method: "post" });
  };

  return (
    <>
      <PageHeader title="Config" />
      <div className="px-4 pb-8 md:px-8 space-y-4">
        <Card>
          <CardContent className="pt-6">
            <fieldset>
              <legend className="mb-3 text-sm font-medium">API Host</legend>
              <RadioGroup defaultValue={host} onValueChange={onHostChange}>
                {API_HOSTS.map((api_host) => (
                  <div key={api_host.value} className="flex items-center space-x-3 py-2">
                    <RadioGroupItem value={api_host.value} id={`host-${api_host.value}`} />
                    <Label htmlFor={`host-${api_host.value}`} className="text-sm font-normal">
                      {api_host.name}{" "}
                      <span className="text-muted-foreground">{new URL(api_host.value).hostname}</span>
                    </Label>
                  </div>
                ))}
              </RadioGroup>
            </fieldset>
          </CardContent>
        </Card>
        <Card>
          <CardContent className="pt-6">
            <Label htmlFor="api-token" className="mb-3 text-sm font-medium">
              API Token
            </Label>
            <input
              id="api-token"
              type="password"
              defaultValue={token}
              onBlur={onTokenBlur}
              placeholder="Leave empty if auth is disabled"
              className="mt-2 flex h-9 w-full rounded-md border border-input bg-transparent px-3 py-1 text-sm shadow-sm transition-colors placeholder:text-muted-foreground focus-visible:outline-none focus-visible:ring-1 focus-visible:ring-ring"
            />
          </CardContent>
        </Card>
      </div>
    </>
  );
}
