import { Tab, Tabs } from '@heroui/react';
import { Folder, Scaling, SlidersHorizontal } from 'lucide-react';
import CompressionOptions from '@/components/side-panel/CompressionOptions.tsx';
import { useTranslation } from 'react-i18next';
import OutputOptions from '@/components/side-panel/OutputOptions.tsx';
import { SIDE_PANEL_TAB } from '@/types.ts';
import useUIStore from '@/stores/ui.store.ts';
import { Key } from 'react';
import ResizeOptions from '@/components/side-panel/ResizeOptions.tsx';

function SidePanel() {
  const { t } = useTranslation();

  const { currentSelectedTab, setCurrentSelectedTab } = useUIStore();
  return (
    <div className="size-full">
      <div className="bg-default-50 size-full overflow-x-auto overflow-y-hidden rounded">
        <Tabs
          classNames={{
            tabList: 'w-full p-0.5 h-[32px] rounded-b-none',
            tabWrapper: 'h-full',
            panel: 'w-full h-[calc(100%_-_32px)] p-0',
            base: 'w-full',
          }}
          // placement="end"
          selectedKey={currentSelectedTab}
          size="sm"
          onSelectionChange={(key: Key) => setCurrentSelectedTab(key as SIDE_PANEL_TAB)}
          // variant="bordered"
        >
          <Tab
            key={SIDE_PANEL_TAB.COMPRESSION}
            title={
              <div className="flex items-center gap-1">
                <SlidersHorizontal className="size-4"></SlidersHorizontal>
                <span className="text-xs font-semibold">{t('compression_options.compression')}</span>
              </div>
            }
          >
            <CompressionOptions></CompressionOptions>
          </Tab>
          <Tab
            key={SIDE_PANEL_TAB.RESIZE}
            title={
              <div className="flex items-center gap-2">
                <Scaling className="size-4"></Scaling>
                <span className="text-xs font-semibold">{t('compression_options.resize')}</span>
              </div>
            }
          >
            <ResizeOptions></ResizeOptions>
          </Tab>
          <Tab
            key={SIDE_PANEL_TAB.OUTPUT}
            title={
              <div className="flex items-center gap-2">
                <Folder className="size-4"></Folder>
                <span className="text-xs font-semibold">{t('compression_options.output')}</span>
              </div>
            }
          >
            <OutputOptions></OutputOptions>
          </Tab>
        </Tabs>
      </div>
    </div>
  );
}

export default SidePanel;
