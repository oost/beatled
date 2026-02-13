import { useLoaderData, useSubmit } from "react-router-dom";
import type { ActionFunctionArgs } from "react-router-dom";
import PageHeader from "../components/page-header";
import { Card, CardContent } from "@/components/ui/card";
import { RadioGroup, RadioGroupItem } from "@/components/ui/radio-group";
import { Label } from "@/components/ui/label";
import { getProgram, postProgram, type ProgramInfo } from "../lib/program";

export async function loader() {
  console.log("Loading programs");
  const programInfo = await getProgram();
  return programInfo;
}

export async function action({ request }: ActionFunctionArgs) {
  const formData = await request.formData();
  const updates = Object.fromEntries(formData);
  await postProgram(parseInt(updates.programId as string));
  return true;
}

export default function ProgramPage() {
  const submit = useSubmit();
  const { programId, programs } = useLoaderData() as ProgramInfo;

  const onValueChange = (value: string) => {
    const formData = new FormData();
    formData.set("programId", value);
    submit(formData, { method: "post" });
  };

  return (
    <>
      <PageHeader title="Program" />
      <div className="px-4 pb-8 md:px-8">
        <Card>
          <CardContent className="pt-6">
            <RadioGroup value={String(programId)} onValueChange={onValueChange}>
              {programs?.map((program) => (
                <div key={program.id} className="flex items-center space-x-3 py-2">
                  <RadioGroupItem value={String(program.id)} id={`program-${program.id}`} />
                  <Label htmlFor={`program-${program.id}`} className="text-sm font-normal">
                    {program.name}
                  </Label>
                </div>
              ))}
            </RadioGroup>
          </CardContent>
        </Card>
      </div>
    </>
  );
}
