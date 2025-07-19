import { Pagination } from '@heroui/react';
import useFileListStore from '@/stores/file-list.store.ts';

function FileListPagination() {
  const { currentPage, setCurrentPage } = useFileListStore();

  const totalPages = useFileListStore.getState().totalPages();

  const pagination =
    totalPages > 0 ? (
      <Pagination
        disableAnimation
        showControls
        classNames={{
          wrapper: 'shadow-none',
          base: 'p-0 m-0',
        }}
        page={currentPage}
        showShadow={false}
        size="sm"
        total={totalPages}
        variant="light"
        onChange={(page) => {
          setCurrentPage(page);
        }}
      />
    ) : null;
  return <div className="flex w-full justify-center">{pagination}</div>;
}

export default FileListPagination;
