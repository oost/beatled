import PanelHeader from "../components/PanelHeader";

export async function loader({ request }) {
  console.log("Loading status");

  return {};
}

export async function action({ request, params }) {
  return true;
}

export default function ProgramPage() {
  return (
    <>
      <PanelHeader
        content={
          <div className="header text-center">
            <h2 className="title">Program</h2>
          </div>
        }
      />
      <div className="content"></div>
    </>
  );
}
