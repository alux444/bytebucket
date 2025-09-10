import React from "react";

export interface ActionBarProps {
  handleFileUpload: (event: React.ChangeEvent<HTMLInputElement>) => void;
  setDisplayCreateModal: (show: boolean) => void;
  refetch: () => void;
  isCreatingFolder: boolean;
  isUploadingFile: boolean;
  isContentsLoading: boolean;
}

const ActionBar = ({ 
  handleFileUpload, 
  setDisplayCreateModal, 
  refetch, 
  isCreatingFolder, 
  isUploadingFile, 
  isContentsLoading 
}: ActionBarProps) => {
  return (
    <div className="action-bar">
      <label htmlFor="file-upload" className="upload-button">
        📤 Upload Files
        <input 
          id="file-upload" 
          type="file" 
          multiple 
          onChange={handleFileUpload} 
          style={{ display: "none" }} 
          disabled={isUploadingFile} 
        />
      </label>

      <button 
        onClick={() => setDisplayCreateModal(true)} 
        className="create-folder-button" 
        disabled={isCreatingFolder}
      >
        📁 New Folder
      </button>

      <button 
        onClick={() => refetch()} 
        className="refresh-button" 
        disabled={isContentsLoading}
      >
        🔄 Refresh
      </button>
    </div>
  );
};

export default ActionBar;
