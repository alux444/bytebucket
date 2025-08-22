import { useState } from "react";
import { useUploadFiles, useCreateFolder, useDownloadFile } from "./useApi";
import type { CreateFolderRequest } from "../api/types";

export const useFileUpload = () => {
  const [uploadProgress, setUploadProgress] = useState<number>(0);
  const [isUploading, setIsUploading] = useState<boolean>(false);

  const uploadMutation = useUploadFiles();

  const uploadFiles = async (files: File[], folderId?: number) => {
    setIsUploading(true);
    setUploadProgress(0);

    try {
      // TODO: real stream progress
      const progressInterval = setInterval(() => {
        setUploadProgress((prev) => {
          if (prev >= 90) {
            clearInterval(progressInterval);
            return prev;
          }
          return prev + 10;
        });
      }, 100);

      const result = await uploadMutation.mutateAsync({
        files,
        folder_id: folderId,
      });

      clearInterval(progressInterval);
      setUploadProgress(100);

      // Reset progress after a short delay
      setTimeout(() => {
        setUploadProgress(0);
        setIsUploading(false);
      }, 1000);

      return result;
    } catch (error) {
      setIsUploading(false);
      setUploadProgress(0);
      throw error;
    }
  };

  return {
    uploadFiles,
    uploadProgress,
    isUploading,
    error: uploadMutation.error,
    isError: uploadMutation.isError,
    reset: uploadMutation.reset,
  };
};

export const useFolderCreation = () => {
  const [validationError, setValidationError] = useState<string | null>(null);

  const createMutation = useCreateFolder();

  const createFolder = async (folderData: CreateFolderRequest) => {
    setValidationError(null);

    if (!folderData.name || folderData.name.trim().length === 0) {
      setValidationError("Folder name cannot be empty");
      return;
    }

    if (folderData.name.length > 255) {
      setValidationError("Folder name cannot exceed 255 characters");
      return;
    }

    return await createMutation.mutateAsync({
      name: folderData.name.trim(),
      parent_id: folderData.parent_id,
    });
  };

  return {
    createFolder,
    validationError,
    isLoading: createMutation.isPending,
    error: createMutation.error,
    isError: createMutation.isError,
    reset: () => {
      setValidationError(null);
      createMutation.reset();
    },
  };
};

export const useFileDownloads = () => {
  const [downloadingFiles, setDownloadingFiles] = useState<Set<number>>(new Set());

  const downloadMutation = useDownloadFile();

  const downloadFile = async (fileId: number, filename?: string) => {
    setDownloadingFiles((prev) => new Set(prev).add(fileId));

    try {
      await downloadMutation.mutateAsync({ fileId, filename });
    } finally {
      setDownloadingFiles((prev) => {
        const next = new Set(prev);
        next.delete(fileId);
        return next;
      });
    }
  };

  const downloadMultipleFiles = async (files: Array<{ id: number; name?: string }>) => {
    const downloadPromises = files.map((file) => downloadFile(file.id, file.name));

    try {
      await Promise.all(downloadPromises);
    } catch (error) {
      console.error("Some files failed to download:", error);
      throw error;
    }
  };

  const isDownloading = (fileId: number) => downloadingFiles.has(fileId);
  const hasActiveDownloads = downloadingFiles.size > 0;

  return {
    downloadFile,
    downloadMultipleFiles,
    isDownloading,
    hasActiveDownloads,
    downloadingFiles: Array.from(downloadingFiles),
    error: downloadMutation.error,
    isError: downloadMutation.isError,
    reset: downloadMutation.reset,
  };
};
