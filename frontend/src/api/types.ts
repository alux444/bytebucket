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
  name: string;
  folder_id: number;
  size: number;
  content_type: string;
  storage_id: string;
  created_at: string;
  updated_at: string;
  tags: string[];
  metadata: Record<string, string>;
}

export interface FolderContentsResponse {
  folder: FolderInfo;
  subfolders: SubfolderItem[];
  files: FileItem[];
}
export interface UploadResponse {
  files: FileResponse[];
}

// Requests
export interface GetFolderRequest {
  folder_id?: number; // default is root
}

export interface CreateFolderRequest {
  name: string;
  parent_id?: number;
}

export interface UploadFilesRequest {
  files: File[];
  folder_id?: number;
}

export interface CreateTagRequest {
  name: string;
}

export interface AddFileTagRequest {
  tagName: string;
}

export type AddFileMetadataRequest = Record<string, string>;

// Entities

export interface FolderInfo {
  id: number | null;
  name: string;
  parent_id: number | null;
}

export interface SubfolderItem {
  id: number;
  name: string;
  parent_id: number | null;
}

export interface FileItem {
  id: number;
  name: string;
  folder_id: number;
  size: number;
  content_type: string;
  storage_id: string;
  created_at: string;
  updated_at: string;
  tags: FileTag[];
  metadata: FileMetadata[];
}

export interface FileTag {
  name: string;
}

export interface FileMetadata {
  key: string;
  value: string;
}

export interface FileRecord {
  id: number;
  name: string;
  folder_id: number;
  size: number;
  content_type: string;
  storage_id: string;
  created_at: string;
}

export interface FolderRecord {
  id: number;
  name: string;
  parent_id: number | null;
  created_at: string;
}
