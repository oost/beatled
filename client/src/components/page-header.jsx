export default function PageHeader({ title }) {
  return (
    <div className="px-4 pb-2 pt-6 md:px-8">
      <h1 className="text-xl font-semibold tracking-tight">{title}</h1>
    </div>
  );
}
