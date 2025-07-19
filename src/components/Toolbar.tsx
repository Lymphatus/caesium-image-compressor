import { Delete, Ellipsis, FolderPlus, ImagePlus, Play, Search, Settings, Trash2 } from 'lucide-react';
import useFileListStore from '@/stores/file-list.store.ts';
import { invoke } from '@tauri-apps/api/core';
import { useTranslation } from 'react-i18next';
import useUIStore from '@/stores/ui.store.ts';
import { Button, Divider, Dropdown, DropdownTrigger } from '@heroui/react';
import AppMenu from '@/components/AppMenu.tsx';
import usePreviewStore from '@/stores/preview.store.ts';

function Toolbar() {
  const { openPickerDialogs, fileList, selectedItems, invokeCompress } = useFileListStore();
  const { setSettingsDialogOpen, showLabelsInToolbar } = useUIStore();
  const { invokePreview } = usePreviewStore();
  const { t } = useTranslation();

  return (
    <div className="bg-content1 flex h-[40px] w-full items-center justify-between px-2">
      <div className="flex h-full items-center gap-1">
        <Button
          disableRipple
          isIconOnly={!showLabelsInToolbar}
          size="sm"
          title={t('actions.add_dots')}
          variant="light"
          onPress={() => openPickerDialogs('files')}
        >
          <ImagePlus className="size-5"></ImagePlus>
          {showLabelsInToolbar && <span>{t('actions.add_dots')}</span>}
        </Button>
        <Button
          disableRipple
          isIconOnly={!showLabelsInToolbar}
          size="sm"
          title={t('actions.add_folder_dots')}
          variant="light"
          onPress={() => openPickerDialogs('folder')}
        >
          <FolderPlus className="size-5"></FolderPlus>
          {showLabelsInToolbar && <span>{t('actions.add_folder_dots')}</span>}
        </Button>
        <div className="h-full py-2">
          <Divider orientation="vertical"></Divider>
        </div>
        <Button
          disableRipple
          color="danger"
          isDisabled={selectedItems.length === 0}
          isIconOnly={!showLabelsInToolbar}
          size="sm"
          title={t('actions.remove')}
          variant="light"
          onPress={async () => await invoke('remove_items_from_list', { keys: selectedItems.map((c) => c.id) })}
        >
          <Delete className="size-5"></Delete>
          {showLabelsInToolbar && <span>{t('actions.remove')}</span>}
        </Button>
        <Button
          disableRipple
          color="danger"
          isDisabled={fileList.length === 0}
          isIconOnly={!showLabelsInToolbar}
          size="sm"
          title={t('actions.clear')}
          variant="light"
          onPress={async () => await invoke('clear_list')}
        >
          <Trash2 className="size-5"></Trash2>
          {showLabelsInToolbar && <span>{t('actions.clear')}</span>}
        </Button>
        <div className="h-full py-2">
          <Divider orientation="vertical"></Divider>
        </div>
        <Button
          disableRipple
          isDisabled={selectedItems.length === 0}
          isIconOnly={!showLabelsInToolbar}
          size="sm"
          title={t('actions.preview')}
          variant="light"
          onPress={() => invokePreview(selectedItems.map((c) => c.id))}
        >
          <Search className="size-5"></Search>
          {showLabelsInToolbar && <span>{t('actions.preview')}</span>}
        </Button>
        <Button
          disableRipple
          color="primary"
          isDisabled={fileList.length === 0}
          isIconOnly={!showLabelsInToolbar}
          size="sm"
          title={t('actions.compress')}
          variant="light"
          onPress={() => invokeCompress()}
        >
          <Play className="size-5"></Play>
          {showLabelsInToolbar && <span>{t('actions.compress')}</span>}
        </Button>
      </div>
      <div className="flex h-full items-center gap-1">
        <Button
          disableRipple
          isIconOnly={!showLabelsInToolbar}
          size="sm"
          title={t('actions.settings')}
          variant="light"
          onPress={() => setSettingsDialogOpen(true)}
        >
          <Settings className="size-5"></Settings>
          {showLabelsInToolbar && <span>{t('actions.settings')}</span>}
        </Button>
        <Dropdown
          classNames={{
            content: 'bg-default-50',
          }}
        >
          <DropdownTrigger>
            <Button disableRipple isIconOnly size="sm" title={t('actions.menu')} variant="light" onPress={() => {}}>
              <Ellipsis className="size-5"></Ellipsis>
            </Button>
          </DropdownTrigger>
          <AppMenu></AppMenu>
        </Dropdown>
      </div>
    </div>
  );
}

export default Toolbar;
