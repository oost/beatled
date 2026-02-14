import React from "react";
import { useLocation, Outlet } from "react-router-dom";
import BottomNav from "./bottom-nav";

export default function RootContainer() {
  const location = useLocation();

  React.useEffect(() => {
    window.scrollTo(0, 0);
  }, [location]);

  return (
    <div className="min-h-screen bg-background">
      <main className="mx-auto max-w-5xl pb-20">
        <Outlet />
      </main>
      <BottomNav />
    </div>
  );
}
