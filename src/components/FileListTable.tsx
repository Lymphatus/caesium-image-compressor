import { Button, Spinner, Table, TableBody, TableCell, TableColumn, TableHeader, TableRow } from '@heroui/react';
import useFileListStore from '@/stores/file-list.store.ts';
import {
  ArrowDown,
  ArrowUp,
  Circle,
  CircleAlert,
  CircleCheck,
  CircleX,
  Delete,
  LoaderCircle,
  Search,
} from 'lucide-react';
import prettyBytes from 'pretty-bytes';
import usePreviewStore from '@/stores/preview.store.ts';
import { useTranslation } from 'react-i18next';
import { sep } from '@tauri-apps/api/path';
import { Selection } from '@react-types/shared';
import { invoke } from '@tauri-apps/api/core';
import { CImage, IMAGE_STATUS } from '@/types.ts';
import { getSavedPercentage } from '@/utils/utils.ts';

function getSubpart(baseFolder: string | null, fullPath: string, filename: string) {
  if (!baseFolder) {
    return '';
  }
  const separator = sep();
  return fullPath.replace(baseFolder, '').replace(filename, '').replace(separator, '');
}

function StatusIcon({ cImage }: { cImage: CImage }) {
  if (cImage.status === IMAGE_STATUS.SUCCESS) {
    return <CircleCheck className="text-success size-4" />;
  } else if (cImage.status === IMAGE_STATUS.ERROR) {
    return <CircleX className="text-danger size-4" />;
  } else if (cImage.status === IMAGE_STATUS.WARNING) {
    return <CircleAlert className="text-warning size-4" />;
  } else if (cImage.status === IMAGE_STATUS.COMPRESSING) {
    return <LoaderCircle className="text-primary size-4 animate-spin" />;
  }
  return <Circle className="text-primary size-4"></Circle>;
}

function SavedLabel({ cImage }: { cImage: CImage }) {
  if (cImage.compressed_size === 0 || cImage.size === cImage.compressed_size) {
    return <span className="text-default-400 text-nowrap">&nbsp;</span>;
  }
  const saved = getSavedPercentage(cImage.size, cImage.compressed_size);
  const textColor = saved < 0 ? 'text-danger' : 'text-success';
  const icon = saved < 0 ? <ArrowUp className="size-4" /> : <ArrowDown className="size-4" />;
  return (
    <span className={textColor + ' flex items-center gap-0.5 text-nowrap'}>
      {icon}
      <span>{cImage.compressed_size !== 0 ? saved * -1 : 0}%</span>
    </span>
  );
}

function FileListTable() {
  const {
    fileList,
    isListLoading,
    baseFolder,
    setSelectedItems,
    selectedItems,
    setCurrentSorting,
    currentSorting,
    setIsListLoading,
  } = useFileListStore();
  const { setCurrentPreviewedCImage, invokePreview, currentPreviewedCImage } = usePreviewStore();

  const { t } = useTranslation();

  const handleSelectionChange = function (keys: Selection) {
    const selectedItems = fileList.filter((item) => (keys === 'all' ? true : keys.has(item.id)));
    setSelectedItems(selectedItems);
  };

  return (
    <Table
      fullWidth
      isHeaderSticky
      removeWrapper
      aria-label="File list"
      checkboxesProps={{ disableAnimation: true, size: 'sm', className: 'p-0 pb-1' }}
      classNames={{
        base: 'h-full justify-between overflow-auto',
        th: 'h-8 first:rounded-b-none first:rounded-t-sm last:rounded-b-none last:rounded-t-sm [&:first-child]:w-[32px]',
        td: 'text-nowrap',
      }}
      layout="auto"
      radius="sm"
      selectedKeys={selectedItems.map((item) => item.id)}
      selectionMode="multiple"
      shadow="none"
      sortDescriptor={currentSorting}
      onRowAction={(key) => setCurrentPreviewedCImage(fileList.find((cImage) => cImage.id === key) || null)}
      onSelectionChange={handleSelectionChange}
      onSortChange={(sort) => {
        setIsListLoading(true);
        setCurrentSorting(sort);
        invoke('sort_list', {
          column: sort.column,
          order: sort.direction,
        })
          .then(() => {})
          .catch(() => {})
          .finally(() => setIsListLoading(false));
      }}
    >
      <TableHeader className="rounded-sm">
        <TableColumn key="status" align="center" width={40}>
          &nbsp;
        </TableColumn>
        <TableColumn key="filename" allowsSorting>
          {t('file_list.filename')}
        </TableColumn>
        <TableColumn key="size" allowsSorting>
          {t('file_list.size')}
        </TableColumn>
        <TableColumn key="resolution" allowsSorting>
          {t('file_list.resolution')}
        </TableColumn>
        <TableColumn key="saved" allowsSorting>
          {t('file_list.saved')}
        </TableColumn>
        <TableColumn key="info">{t('file_list.additional_info')}</TableColumn>
        <TableColumn key="actions" width={40}>
          {t('file_list.actions')}
        </TableColumn>
      </TableHeader>
      <TableBody
        isLoading={isListLoading}
        // items={fileList}
        loadingContent={
          <div className="bg-background/50 z-10 flex size-full items-center justify-center">
            <Spinner />
          </div>
        }
      >
        {fileList.map((cImage) => (
          <TableRow key={cImage.id} className={cImage.id === currentPreviewedCImage?.id ? 'bg-default-100' : ''}>
            <TableCell>
              <div className="flex items-center justify-center">
                <StatusIcon cImage={cImage}></StatusIcon>
              </div>
            </TableCell>
            <TableCell>
              <span className="text-nowrap">
                <small className="text-default-400">{getSubpart(baseFolder, cImage.path, cImage.name)}</small>
                <span>{cImage.name}</span>
              </span>
            </TableCell>
            <TableCell>
              <div className="flex flex-nowrap items-center gap-1">
                <span
                  className={
                    cImage.compressed_size && cImage.compressed_size !== cImage.size
                      ? 'text-default-400 text-nowrap line-through'
                      : 'text-nowrap'
                  }
                >
                  {prettyBytes(cImage.size)}
                </span>
                {cImage.compressed_size !== 0 && cImage.size !== cImage.compressed_size && (
                  <span className="text-nowrap">{prettyBytes(cImage.compressed_size)}</span>
                )}
              </div>
            </TableCell>
            <TableCell>
              <div className="flex size-full flex-nowrap items-center gap-1">
                <span
                  className={
                    cImage.compressed_width !== 0 &&
                    cImage.compressed_height !== 0 &&
                    (cImage.compressed_width !== cImage.width || cImage.compressed_height !== cImage.height)
                      ? 'text-default-400 text-nowrap line-through'
                      : 'text-nowrap'
                  }
                >{`${cImage.width}x${cImage.height}`}</span>
                {cImage.compressed_width !== 0 &&
                  cImage.compressed_height !== 0 &&
                  (cImage.compressed_width !== cImage.width || cImage.compressed_height !== cImage.height) && (
                    <span className="text-nowrap">{`${cImage.compressed_width}x${cImage.compressed_height}`}</span>
                  )}
              </div>
            </TableCell>
            <TableCell>
              <SavedLabel cImage={cImage} />
            </TableCell>
            <TableCell>
              <span className="text-nowrap">{cImage.info}</span>
            </TableCell>
            <TableCell>
              <div className="flex items-center justify-between gap-1">
                <Button
                  disableRipple
                  isIconOnly
                  isDisabled={cImage.status === IMAGE_STATUS.COMPRESSING}
                  size="sm"
                  title={t('actions.preview')}
                  variant="light"
                  onPress={() => invokePreview([cImage.id])}
                >
                  <Search className="size-4"></Search>
                </Button>
                <Button
                  disableRipple
                  isIconOnly
                  color="danger"
                  isDisabled={cImage.status === IMAGE_STATUS.COMPRESSING}
                  size="sm"
                  title={t('actions.remove')}
                  variant="light"
                  onPress={async () => {
                    await invoke('remove_items_from_list', { keys: [cImage.id] });
                  }}
                >
                  <Delete className="size-4"></Delete>
                </Button>
              </div>
            </TableCell>
          </TableRow>
        ))}
      </TableBody>
    </Table>
  );
}

export default FileListTable;
