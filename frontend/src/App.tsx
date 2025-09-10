import React, { useState } from "react";
import { useHealth, useFolderNavigation, useFileUpload, useFolderCreation, useFileDownloads } from "./hooks";
import "./App.css";
import Header from "./components/Header";
import UploadProgress from "./components/UploadProgress";
import NavigationBreadcrumb from "./components/NavigationBreadcrumb";
import ActionBar from "./components/ActionBar";
import ErrorMessages from "./components/ErrorMessages";
import FileExplorerGrid from "./components/file-explorer/FileExplorerGrid";
import CreateFolderModal from "./components/modals/CreateFolderModal";

const App: React.FC = () => {
  const [showCreateModal, setShowCreateModal] = useState(false);
  const [folderName, setFolderName] = useState("");

  const { data: health, isLoading: healthLoading } = useHealth();

  const { 
    currentFolderId, 
    navigationPath, 
    subfolders, 
    files, 
    isLoading: contentsLoading, 
    error: contentsError, 
    navigateToFolder, 
    navigateToIndexInPath, 
    refetch 
  } = useFolderNavigation();

  const { uploadFiles, uploadProgress, isUploading, error: uploadError } = useFileUpload();
  const { createFolder, validationError, isLoading: creatingFolder, error: createError } = useFolderCreation();
  const { downloadFile, isDownloading } = useFileDownloads();

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
      <Header health={health} />

      <main className="app-main">
        <NavigationBreadcrumb 
          navigationPath={navigationPath} 
          navigateToIndexInPath={navigateToIndexInPath} 
        />

        <ActionBar
          handleFileUpload={handleFileUpload}
          setDisplayCreateModal={setShowCreateModal}
          refetch={refetch}
          isCreatingFolder={creatingFolder}
          isUploadingFile={isUploading}
          isContentsLoading={contentsLoading}
        />

        {isUploading && <UploadProgress uploadProgress={uploadProgress} />}

        <ErrorMessages 
          uploadError={uploadError}
          createError={createError}
          contentsError={contentsError}
        />

        <FileExplorerGrid
          subfolders={subfolders}
          files={files}
          isLoading={contentsLoading}
          navigateToFolder={navigateToFolder}
          handleDownload={handleDownload}
          isDownloading={isDownloading}
        />
      </main>

      <CreateFolderModal
        showCreateModal={showCreateModal}
        folderName={folderName}
        setFolderName={setFolderName}
        handleCreateFolder={handleCreateFolder}
        setShowCreateModal={setShowCreateModal}
        validationError={validationError || undefined}
        isCreatingFolder={creatingFolder}
      />
    </div>
  );
};

export default App;
