import { formatFileSize, getFileIcon } from "../../util/ui";

export interface FileItem {
  id: number;
  name: string;
  size: number;
  content_type: string;
}

export interface FolderItem {
  id: number;
  name: string;
}

export interface FileExplorerGridProps {
  subfolders: FolderItem[];
  files: FileItem[];
  isLoading: boolean;
  navigateToFolder: (folderId: number, folderName: string) => void;
  handleDownload: (fileId: number, filename: string) => void;
  isDownloading: (fileId: number) => boolean;
}

const FileExplorerGrid = ({ 
  subfolders, 
  files, 
  isLoading, 
  navigateToFolder, 
  handleDownload, 
  isDownloading 
}: FileExplorerGridProps) => {
  if (isLoading) {
    return <div className="loading">Loading folder contents...</div>;
  }

  return (
    <div className="file-grid">
      {/* Subfolders */}
      {subfolders.map((folder) => (
        <div 
          key={`folder-${folder.id}`} 
          className="file-item folder" 
          onDoubleClick={() => navigateToFolder(folder.id, folder.name)}
        >
          <div className="file-icon">📁</div>
          <div className="file-name">{folder.name}</div>
          <div className="file-info">Folder</div>
        </div>
      ))}

      {/* Files */}
      {files.map((file) => (
        <div 
          key={`file-${file.id}`} 
          className="file-item file" 
          onDoubleClick={() => handleDownload(file.id, file.name)}
        >
          <div className="file-icon">{getFileIcon(file.content_type)}</div>
          <div className="file-name">{file.name}</div>
          <div className="file-info">
            {formatFileSize(file.size)}
            {isDownloading(file.id) && <span className="downloading"> - Downloading...</span>}
          </div>
        </div>
      ))}

      {/* Empty State */}
      {subfolders.length === 0 && files.length === 0 && (
        <div className="empty-state">
          <p>This folder is empty</p>
          <p>Upload files or create folders to get started</p>
        </div>
      )}
    </div>
  );
};

export default FileExplorerGrid;