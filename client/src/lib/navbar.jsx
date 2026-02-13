import { Activity, Music, ScrollText, Settings } from "lucide-react";

export const NAVBAR_ROUTES = [
  {
    name: "Status",
    path: "/status",
    icon: <Activity className="h-5 w-5" />,
  },
  {
    name: "Program",
    path: "/program",
    icon: <Music className="h-5 w-5" />,
  },
  {
    name: "Log",
    path: "/log",
    icon: <ScrollText className="h-5 w-5" />,
  },
  {
    name: "Config",
    path: "/config",
    icon: <Settings className="h-5 w-5" />,
  },
];
