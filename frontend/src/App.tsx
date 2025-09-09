import React, { useState } from "react";
import { useHealth, useFolderNavigation, useFileUpload, useFolderCreation, useFileDownloads } from "./hooks";
import "./App.css";
import { formatFileSize, getFileIcon } from "./util/ui";

// TODO: rewrite this in components lol
const App: React.FC = () => {
  const [showCreateModal, setShowCreateModal] = useState(false);
  const [folderName, setFolderName] = useState("");

  // Health check
  const { data: health, isLoading: healthLoading } = useHealth();

  // Folder navigation using real API
  const { currentFolderId, navigationPath, subfolders, files, isLoading: contentsLoading, error: contentsError, navigateToFolder, navigateToIndexInPath, refetch } = useFolderNavigation();

  // File operations
  const { uploadFiles, uploadProgress, isUploading, error: uploadError } = useFileUpload();
  const { createFolder, validationError, isLoading: creatingFolder, error: createError } = useFolderCreation();
  const { downloadFile, isDownloading } = useFileDownloads();

  // Handle file upload
  const handleFileUpload = async (event: React.ChangeEvent<HTMLInputElement>) => {
    const selectedFiles = event.target.files;
    if (!selectedFiles || selectedFiles.length === 0) return;

    try {
      await uploadFiles(Array.from(selectedFiles), currentFolderId || undefined);
      event.target.value = ""; // Reset input
    } catch (error) {
      console.error("Upload failed:", error);
    }
  };

  // Handle folder creation
  const handleCreateFolder = async () => {
    if (!folderName.trim()) return;

    try {
      await createFolder({
        name: folderName,
        parent_id: currentFolderId || undefined,
      });
      setFolderName("");
      setShowCreateModal(false);
    } catch (error) {
      console.error("Failed to create folder:", error);
    }
  };

  // Handle file download
  const handleDownload = async (fileId: number, filename: string) => {
    try {
      await downloadFile(fileId, filename);
    } catch (error) {
      console.error("Download failed:", error);
    }
  };

  if (healthLoading) {
    return <div className="loading">Loading ByteBucket...</div>;
  }

  return (
    <div className="app">
      <header className="app-header">
        <h1>🪣 ByteBucket File Explorer</h1>
        <div className="server-status">
          <span className={`status-indicator ${health?.status === "healthy" ? "healthy" : "unhealthy"}`}></span>
          Server: {health?.status || "Unknown"}
        </div>
      </header>

      <main className="app-main">
        {/* Navigation Breadcrumb */}
        <nav className="breadcrumb">
          {navigationPath.map((item, index) => (
            <span key={`${item.id}-${index}`}>
              <button className="breadcrumb-item" onClick={() => navigateToIndexInPath(index)} disabled={index === navigationPath.length - 1}>
                {item.name}
              </button>
              {index < navigationPath.length - 1 && <span className="breadcrumb-separator">›</span>}
            </span>
          ))}
        </nav>

        {/* Action Bar */}
        <div className="action-bar">
          <label htmlFor="file-upload" className="upload-button">
            📤 Upload Files
            <input id="file-upload" type="file" multiple onChange={handleFileUpload} style={{ display: "none" }} disabled={isUploading} />
          </label>

          <button onClick={() => setShowCreateModal(true)} className="create-folder-button" disabled={creatingFolder}>
            📁 New Folder
          </button>

          <button onClick={() => refetch()} className="refresh-button" disabled={contentsLoading}>
            🔄 Refresh
          </button>
        </div>

        {/* Upload Progress */}
        {isUploading && (
          <div className="upload-progress">
            <div className="progress-bar">
              <div className="progress-fill" style={{ width: `${uploadProgress}%` }}></div>
            </div>
            <span>Uploading... {uploadProgress}%</span>
          </div>
        )}

        {/* Error Messages */}
        {(uploadError || createError || contentsError) && (
          <div className="error-message">
            {uploadError && <p>Upload error: {uploadError.message}</p>}
            {createError && <p>Create folder error: {createError.message}</p>}
            {contentsError && <p>Failed to load folder contents: {contentsError.message}</p>}
          </div>
        )}

        {/* Loading State */}
        {contentsLoading && <div className="loading">Loading folder contents...</div>}

        {/* File Explorer Grid */}
        {!contentsLoading && (
          <div className="file-grid">
            {/* Subfolders */}
            {subfolders.map((folder) => (
              <div key={`folder-${folder.id}`} className="file-item folder" onDoubleClick={() => navigateToFolder(folder.id, folder.name)}>
                <div className="file-icon">📁</div>
                <div className="file-name">{folder.name}</div>
                <div className="file-info">Folder</div>
              </div>
            ))}

            {/* Files */}
            {files.map((file) => (
              <div key={`file-${file.id}`} className="file-item file" onDoubleClick={() => handleDownload(file.id, file.name)}>
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
        )}
      </main>

      {/* Create Folder Modal */}
      {showCreateModal && (
        <div className="modal-overlay">
          <div className="modal">
            <h3>Create New Folder</h3>
            <input type="text" value={folderName} onChange={(e) => setFolderName(e.target.value)} placeholder="Folder name" onKeyDown={(e) => e.key === "Enter" && handleCreateFolder()} autoFocus />
            {validationError && <div className="validation-error">{validationError}</div>}
            <div className="modal-actions">
              <button onClick={handleCreateFolder} disabled={!folderName.trim() || creatingFolder}>
                {creatingFolder ? "Creating..." : "Create"}
              </button>
              <button
                onClick={() => {
                  setShowCreateModal(false);
                  setFolderName("");
                }}
                disabled={creatingFolder}
              >
                Cancel
              </button>
            </div>
          </div>
        </div>
      )}
    </div>
  );
};

export default App;
