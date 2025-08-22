import { useCallback, useState } from "react";
import { useFolderContents } from "./useApi";
import type { FileItem, FolderInfo, SubfolderItem } from "../api";

export interface NavigationItem {
  id: number | null;
  name: string;
}

export const useFolderNavigation = (initialFolderId?: number) => {
  const [currentFolderId, setCurrentFolderId] = useState<number | undefined>(initialFolderId);
  const [navigationPath, setNavigationPath] = useState<NavigationItem[]>([{ id: null, name: "Root" }]);

  const { data: folderContents, isLoading, error, refetch } = useFolderContents(currentFolderId);

  const navigateToFolder = useCallback((folderId: number, folderName: string) => {
    setCurrentFolderId(folderId);
    setNavigationPath((prev) => [...prev, { id: folderId, name: folderName }]);
  }, []);

  const navigateToIndexInPath = useCallback(
    (index: number) => {
      const targetItem = navigationPath[index];
      if (targetItem) {
        setCurrentFolderId(targetItem.id || undefined);
        setNavigationPath(navigationPath.slice(0, index + 1));
      }
    },
    [navigationPath]
  );

  const navigateBack = useCallback(() => {
    if (navigationPath.length > 1) {
      const newPath = navigationPath.slice(0, -1);
      const parentItem = newPath[newPath.length - 1];
      setCurrentFolderId(parentItem.id || undefined);
      setNavigationPath(newPath);
    }
  }, [navigationPath]);

  const navigateToRoot = useCallback(() => {
    setCurrentFolderId(undefined);
    setNavigationPath([{ id: null, name: "Root" }]);
  }, []);

  const currentFolder: FolderInfo | null = folderContents?.folder || null;
  const subfolders: SubfolderItem[] = folderContents?.subfolders || [];
  const files: FileItem[] = folderContents?.files || [];

  const canNavigateBack = navigationPath.length > 1;

  return {
    currentFolderId,
    navigationPath,
    currentFolder,
    subfolders,
    files,
    folderContents,

    isLoading,
    error,
    refetch,

    navigateToFolder,
    navigateBack,
    navigateToIndexInPath,
    navigateToRoot,
    canNavigateBack,
  };
};
