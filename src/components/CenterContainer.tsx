import ListPanel from '@/components/ListPanel.tsx';
import PreviewPanel from '@/components/PreviewPanel.tsx';
import SidePanel from '@/components/side-panel/SidePanel.tsx';
import { Panel, PanelGroup, PanelResizeHandle } from 'react-resizable-panels';
import useUIStore from '@/stores/ui.store.ts';

function CenterContainer() {
  const { splitPanels, setSplitPanels, showPreviewPanel } = useUIStore();

  return (
    <div className="center-container p-1">
      <PanelGroup
        direction="horizontal"
        onLayout={(size) => setSplitPanels({ center: splitPanels.center, main: size })}
      >
        <Panel defaultSize={splitPanels.main[0]} maxSize={80} minSize={20}>
          <PanelGroup
            direction="vertical"
            onLayout={(size) => setSplitPanels({ center: size, main: splitPanels.main })}
          >
            <Panel defaultSize={splitPanels.center[0]} maxSize={80} minSize={20}>
              <ListPanel></ListPanel>
            </Panel>
            {showPreviewPanel && <PanelResizeHandle className="hover:bg-primary h-1 rounded bg-transparent" />}
            {showPreviewPanel && (
              <Panel defaultSize={splitPanels.center[1]} maxSize={80} minSize={20}>
                <PreviewPanel></PreviewPanel>
              </Panel>
            )}
          </PanelGroup>
        </Panel>
        <PanelResizeHandle className="hover:bg-primary w-1 rounded bg-transparent" />
        <Panel defaultSize={splitPanels.main[1]} maxSize={80} minSize={20}>
          <SidePanel></SidePanel>
        </Panel>
      </PanelGroup>
    </div>
  );
}

export default CenterContainer;
