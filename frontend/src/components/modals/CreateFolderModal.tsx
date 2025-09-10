export interface CreateFolderModalProps {
  showCreateModal: boolean;
  folderName: string;
  setFolderName: (name: string) => void;
  handleCreateFolder: () => void;
  setShowCreateModal: (show: boolean) => void;
  validationError?: string;
  isCreatingFolder: boolean;
}

const CreateFolderModal = ({
  showCreateModal,
  folderName,
  setFolderName,
  handleCreateFolder,
  setShowCreateModal,
  validationError,
  isCreatingFolder
}: CreateFolderModalProps) => {
  if (!showCreateModal) {
    return null;
  }

  const handleKeyDown = (e: React.KeyboardEvent<HTMLInputElement>) => {
    if (e.key === "Enter") {
      handleCreateFolder();
    }
  };

  const handleCancel = () => {
    setShowCreateModal(false);
    setFolderName("");
  };

  return (
    <div className="modal-overlay">
      <div className="modal">
        <h3>Create New Folder</h3>
        <input 
          type="text" 
          value={folderName} 
          onChange={(e) => setFolderName(e.target.value)} 
          placeholder="Folder name" 
          onKeyDown={handleKeyDown}
          autoFocus 
        />
        {validationError && <div className="validation-error">{validationError}</div>}
        <div className="modal-actions">
          <button 
            onClick={handleCreateFolder} 
            disabled={!folderName.trim() || isCreatingFolder}
          >
            {isCreatingFolder ? "Creating..." : "Create"}
          </button>
          <button
            onClick={handleCancel}
            disabled={isCreatingFolder}
          >
            Cancel
          </button>
        </div>
      </div>
    </div>
  );
};

export default CreateFolderModal;