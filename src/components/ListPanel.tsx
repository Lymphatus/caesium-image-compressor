import FileListTable from '@/components/FileListTable.tsx';
import FileListPagination from '@/components/FileListPagination.tsx';

function ListPanel() {
  return (
    <div className="size-full">
      <div className="bg-content1 flex size-full flex-col gap-1 rounded">
        <FileListTable></FileListTable>
        <div className="bg-content2 flex min-h-[40px] items-center justify-center p-1">
          <FileListPagination></FileListPagination>
        </div>
      </div>
    </div>
  );
}

export default ListPanel;
