import { DropdownItem, DropdownMenu } from '@heroui/react';
import { useTranslation } from 'react-i18next';
import useUIStore from '@/stores/ui.store.ts';
import { Heart, Import, Info, RefreshCcw } from 'lucide-react';

function AppMenu() {
  const { t } = useTranslation();
  const {
    showPreviewPanel,
    autoPreview,
    showLabelsInToolbar,
    setShowPreviewPanel,
    setAutoPreview,
    setShowLabelsInToolbar,
    getAppMenuSelectedItems,
    setAboutDialogOpen,
    setCheckForUpdatesDialogOpen,
  } = useUIStore();

  return (
    <DropdownMenu aria-label="menu" selectedKeys={getAppMenuSelectedItems()} selectionMode="multiple">
      <DropdownItem key="advancedImport" hideSelectedIcon showDivider startContent={<Import className="size-4" />}>
        {t('actions.advanced_import')}
      </DropdownItem>
      <DropdownItem
        key="showPreview"
        disableAnimation
        closeOnSelect={false}
        startContent={<div className="size-4" />}
        onPress={() => setShowPreviewPanel(!showPreviewPanel)}
      >
        {t('actions.show_previews')}
      </DropdownItem>
      <DropdownItem
        key="autoPreview"
        disableAnimation
        closeOnSelect={false}
        startContent={<div className="size-4" />}
        onPress={() => setAutoPreview(!autoPreview)}
      >
        {t('actions.auto_preview')}
      </DropdownItem>
      <DropdownItem
        key="showToolbarLabels"
        disableAnimation
        showDivider
        closeOnSelect={false}
        startContent={<div className="size-4" />}
        onPress={() => setShowLabelsInToolbar(!showLabelsInToolbar)}
      >
        {t('actions.show_toolbar_labels')}
      </DropdownItem>
      <DropdownItem
        key="donate"
        hideSelectedIcon
        href="https://saerasoft.com/caesium/donate"
        startContent={<Heart className="size-4 text-pink-500" />}
        target="_blank"
      >
        {t('actions.donate')}
      </DropdownItem>
      <DropdownItem
        key="checkForUpdates"
        hideSelectedIcon
        startContent={<RefreshCcw className="size-4" />}
        onPress={() => {
          setCheckForUpdatesDialogOpen(true);
        }}
      >
        {t('actions.check_for_updates')}
      </DropdownItem>
      <DropdownItem
        key="about"
        hideSelectedIcon
        startContent={<Info className="size-4" />}
        onPress={() => setAboutDialogOpen(true)}
      >
        {t('actions.about')}
      </DropdownItem>
    </DropdownMenu>
  );
}

export default AppMenu;
