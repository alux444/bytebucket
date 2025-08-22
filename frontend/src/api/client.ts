import type { ApiError, HealthResponse, FolderResponse, UploadResponse, CreateFolderRequest, UploadFilesRequest, GetFolderRequest, FolderContentsResponse } from "./types";
import { config } from "../config/env";

const API_BASE_URL = config.apiBaseUrl;

export class ApiErrorClass extends Error {
  public status: number;
  public data: ApiError;

  constructor(status: number, data: ApiError) {
    super(data.error);
    this.name = "ApiError";
    this.status = status;
    this.data = data;
  }
}

async function apiRequest<T>(endpoint: string, options: RequestInit = {}): Promise<T> {
  const url = `${API_BASE_URL}${endpoint}`;

  const response = await fetch(url, {
    headers: {
      "Content-Type": "application/json",
      ...options.headers,
    },
    ...options,
  });

  if (!response.ok) {
    let errorData: ApiError;
    try {
      errorData = await response.json();
    } catch {
      errorData = { error: `HTTP ${response.status}: ${response.statusText}` };
    }
    throw new ApiErrorClass(response.status, errorData);
  }

  const contentType = response.headers.get("content-type");
  if (contentType?.includes("application/json")) {
    return response.json();
  }

  // non json
  return response as unknown as T;
}

export const api = {
  health: (): Promise<HealthResponse> => {
    return apiRequest<HealthResponse>("/health");
  },

  root: (): Promise<string> => {
    return apiRequest<string>("/", {
      headers: { "Content-Type": "text/plain" },
    });
  },

  getFolderContents: (params: GetFolderRequest = {}): Promise<FolderContentsResponse> => {
    const endpoint = `/folder/${params.folder_id ? params.folder_id : ""}`;
    return apiRequest<FolderContentsResponse>(endpoint);
  },

  createFolder: (data: CreateFolderRequest): Promise<FolderResponse> => {
    return apiRequest<FolderResponse>("/folder", {
      method: "POST",
      body: JSON.stringify(data),
    });
  },

  uploadFiles: async (data: UploadFilesRequest): Promise<UploadResponse> => {
    const formData = new FormData();

    // Add each file with the field name 'file'
    data.files.forEach((file) => {
      formData.append("file", file);
    });

    if (data.folder_id) {
      formData.append("folder_id", data.folder_id.toString());
    }

    const url = `${API_BASE_URL}/upload`;

    const response = await fetch(url, {
      method: "POST",
      // Don't set Content-Type for FormData, let the browser set it with boundary
      body: formData,
    });

    if (!response.ok) {
      let errorData: ApiError;
      try {
        errorData = await response.json();
      } catch {
        errorData = { error: `HTTP ${response.status}: ${response.statusText}` };
      }
      throw new ApiErrorClass(response.status, errorData);
    }

    return response.json();
  },

  downloadFile: async (fileId: number): Promise<Blob> => {
    const url = `${API_BASE_URL}/download/${fileId}`;

    const response = await fetch(url);

    if (!response.ok) {
      let errorData: ApiError;
      try {
        errorData = await response.json();
      } catch {
        errorData = { error: `HTTP ${response.status}: ${response.statusText}` };
      }
      throw new ApiErrorClass(response.status, errorData);
    }

    return response.blob();
  },

  triggerFileDownload: async (fileId: number, filename?: string): Promise<void> => {
    const blob = await api.downloadFile(fileId);

    const url = window.URL.createObjectURL(blob);
    const link = document.createElement("a");
    link.href = url;
    link.download = filename || `file_${fileId}`;

    document.body.appendChild(link);
    link.click();

    document.body.removeChild(link);
    window.URL.revokeObjectURL(url);
  },
};
