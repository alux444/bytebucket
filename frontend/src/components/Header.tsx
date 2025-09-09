import type { HealthResponse } from "../api";

export type HeaderProps = HealthResponse | undefined;

const Header = ({ health }: { health: HeaderProps }) => {
  return (
    <header className="app-header">
      <h1>🪣 ByteBucket File Explorer</h1>
      <div className="server-status">
        <span className={`status-indicator ${health?.status === "healthy" ? "healthy" : "unhealthy"}`}></span>
        Server: {health?.status || "Unknown"}
      </div>
    </header>
  );
};

export default Header;
