import { useMutation, useQuery, useQueryClient } from "@tanstack/react-query";
import { api } from "../api/client";
import type { CreateFolderRequest, UploadFilesRequest, FolderResponse, UploadResponse, HealthResponse } from "../api/types";

export const queryKeys = {
  health: ["health"] as const,
  root: ["root"] as const,
  folders: ["folders"] as const,
  files: ["files"] as const,
} as const;

export const useHealth = () => {
  return useQuery<HealthResponse>({
    queryKey: queryKeys.health,
    queryFn: api.health,
    staleTime: 30000,
    retry: 3,
  });
};

export const useRoot = () => {
  return useQuery<string>({
    queryKey: queryKeys.root,
    queryFn: api.root,
    staleTime: 60000,
  });
};

export const useCreateFolder = () => {
  const queryClient = useQueryClient();

  return useMutation<FolderResponse, Error, CreateFolderRequest>({
    mutationFn: api.createFolder,
    onSuccess: () => {
      // Invalidate folder queries to refetch data
      queryClient.invalidateQueries({
        queryKey: queryKeys.folders,
      });
    },
    onError: (error) => {
      console.error("Failed to create folder:", error);
    },
  });
};

export const useUploadFiles = () => {
  const queryClient = useQueryClient();

  return useMutation<UploadResponse, Error, UploadFilesRequest>({
    mutationFn: api.uploadFiles,
    onSuccess: () => {
      // Invalidate file queries to refetch data
      queryClient.invalidateQueries({
        queryKey: queryKeys.files,
      });
    },
    onError: (error) => {
      console.error("Failed to upload files:", error);
    },
  });
};

export const useDownloadFile = () => {
  return useMutation<void, Error, { fileId: number; filename?: string }>({
    mutationFn: ({ fileId, filename }) => api.triggerFileDownload(fileId, filename),
    onError: (error) => {
      console.error("Failed to download file:", error);
    },
  });
};

export const useDownloadFileBlob = () => {
  return useMutation<Blob, Error, number>({
    mutationFn: api.downloadFile,
    onError: (error) => {
      console.error("Failed to download file blob:", error);
    },
  });
};
