import { BsGearWideConnected, BsMusicPlayer } from "react-icons/bs";
import { HiOutlineClipboardDocumentList } from "react-icons/hi2";

export const NAVBAR_ROUTES = [
  { name: "Status", path: "/status", icon: <BsGearWideConnected /> },
  { name: "Program", path: "/program", icon: <BsMusicPlayer /> },
  { name: "Log", path: "/log", icon: <HiOutlineClipboardDocumentList /> },
  { name: "Config", path: "/config", icon: <HiOutlineClipboardDocumentList /> },
];
