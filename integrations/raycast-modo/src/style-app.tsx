import { ActionPanel, Action, List, showToast, Toast } from "@raycast/api";
import { exec } from "child_process";
import { promisify } from "util";
import * as fs from "fs";
import * as path from "path";

const execAsync = promisify(exec);

interface AppItem {
  name: string;
  path: string;
}

export default function Command() {
  const [apps, setApps] = React.useState<AppItem[]>([]);
  const [loading, setLoading] = React.useState(true);

  React.useEffect(() => {
    async function loadApps() {
      const appDirs = ["/Applications", path.join(process.env.HOME || "", "Applications")];
      const found: AppItem[] = [];

      for (const dir of appDirs) {
        if (!fs.existsSync(dir)) continue;
        const files = fs.readdirSync(dir);
        for (const file of files) {
          if (file.endsWith(".app")) {
            found.push({
              name: file.replace(/\.app$/, ""),
              path: path.join(dir, file),
            });
          }
        }
      }
      setApps(found.sort((a, b) => a.name.localeCompare(b.name)));
      setLoading(false);
    }
    loadApps();
  }, []);

  async function handleStyle(app: AppItem, shape: string) {
    const toast = await showToast({ style: Toast.Style.Animated, title: `Styling ${app.name}…` });
    try {
      const modoPath = `${process.env.HOME}/.local/bin/modo`;
      await execAsync(`"${modoPath}" style "${app.path}" --shape ${shape} --bg pastel`);
      toast.style = Toast.Style.Success;
      toast.title = `Styled ${app.name}!`;
    } catch (err: any) {
      toast.style = Toast.Style.Failure;
      toast.title = "Failed to style";
      toast.message = err?.message || String(err);
    }
  }

  return (
    <List isLoading={loading} searchBarPlaceholder="Search installed application to style…">
      {apps.map((app) => (
        <List.Item
          key={app.path}
          title={app.name}
          subtitle={app.path}
          actions={
            <ActionPanel>
              <Action title="Apply Squircle Icon" onAction={() => handleStyle(app, "squircle")} />
              <Action title="Apply Circle Icon" onAction={() => handleStyle(app, "circle")} />
              <Action title="Apply Rounded Rect Icon" onAction={() => handleStyle(app, "roundedRect")} />
              <Action title="Apply Pebble Icon" onAction={() => handleStyle(app, "pebble")} />
              <Action title="Apply Hexagon Icon" onAction={() => handleStyle(app, "hexagon")} />
            </ActionPanel>
          }
        />
      ))}
    </List>
  );
}
