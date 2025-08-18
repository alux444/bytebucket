// Responses
export interface ApiError {
  error: string;
}

export interface HealthResponse {
  status: string;
}

export interface FolderResponse {
  id: number;
  name: string;
  parent_id: number | null;
}

export interface FileResponse {
  id: number;
  storage_id: string;
  filename: string;
  content_type: string;
  size: number;
  folder_id: number;
}

export interface UploadResponse {
  files: FileResponse[];
}

// Requests
export interface CreateFolderRequest {
  name: string;
  parent_id?: number;
}

export interface UploadFilesRequest {
  files: File[];
  folder_id?: number;
}

// Entities
export interface FileRecord {
  id: number;
  name: string;
  folderId: number;
  size: number;
  contentType: string;
  storageId: string;
  createdAt: string;
}

export interface FolderRecord {
  id: number;
  name: string;
  parentId: number | null;
  createdAt: string;
}
