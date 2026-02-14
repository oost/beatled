import { NavLink } from "react-router-dom";
import { cn } from "@/lib/utils";
import { NAVBAR_ROUTES } from "@/lib/navbar";
import { Sun, Moon } from "lucide-react";
import { useTheme } from "@/hooks/use-theme";

export default function BottomNav() {
  const { theme, toggle } = useTheme();

  return (
    <nav className="fixed bottom-0 left-0 right-0 z-50 border-t border-border bg-card/95 backdrop-blur-sm safe-bottom">
      <div className="mx-auto flex h-16 max-w-5xl items-stretch justify-around">
        {NAVBAR_ROUTES.map((route) => (
          <NavLink
            key={route.path}
            to={route.path}
            className={({ isActive }) =>
              cn(
                "relative flex flex-1 flex-col items-center justify-center gap-0.5 text-[11px] font-medium text-muted-foreground transition-colors",
                isActive && "text-primary",
              )
            }
          >
            {({ isActive }) => (
              <>
                {isActive && <span className="absolute top-0 h-0.5 w-8 rounded-full bg-primary" />}
                {route.icon}
                <span>{route.name}</span>
              </>
            )}
          </NavLink>
        ))}
        <button
          type="button"
          onClick={toggle}
          className="flex flex-col items-center justify-center gap-0.5 px-4 text-[11px] font-medium text-muted-foreground transition-colors"
          aria-label="Toggle theme"
        >
          {theme === "dark" ? <Sun className="h-5 w-5" /> : <Moon className="h-5 w-5" />}
          <span>Theme</span>
        </button>
      </div>
    </nav>
  );
}
