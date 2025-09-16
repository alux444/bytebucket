import { useCallback, useState } from "react";
import { useFolderContents } from "./useApi";
import type { FileItem, FolderInfo, SubfolderItem } from "../api";

export interface NavigationItem {
  id: number | null;
  name: string;
}

export const useFolderNavigation = (initialFolderId?: number) => {
  const [currentFolderId, setCurrentFolderId] = useState<number | undefined>(initialFolderId);
  const [navigationPath, setNavigationPath] = useState<NavigationItem[]>([{ id: 1, name: "Root" }]);

  const { data: folderContents, isLoading, error, refetch } = useFolderContents(currentFolderId);

  const navigateToFolder = useCallback((folderId: number, folderName: string) => {
    setCurrentFolderId(folderId);
    setNavigationPath((prev) => [...prev, { id: folderId, name: folderName }]);
  }, []);

  const navigateToIndexInPath = useCallback(
    (index: number) => {
      const targetItem = navigationPath[index];
      if (targetItem) {
        const targetId = targetItem.id === 1 ? undefined : (targetItem.id ?? undefined);
        setCurrentFolderId(targetId);
        setNavigationPath(navigationPath.slice(0, index + 1));
      }
    },
    [navigationPath]
  );

  const navigateBack = useCallback(() => {
    if (navigationPath.length > 1) {
      const newPath = navigationPath.slice(0, -1);
      const parentItem = newPath[newPath.length - 1];
      const parentId = parentItem.id === 1 ? undefined : (parentItem.id ?? undefined);
      setCurrentFolderId(parentId);
      setNavigationPath(newPath);
    }
  }, [navigationPath]);

  const navigateToRoot = useCallback(() => {
    setCurrentFolderId(undefined);
    setNavigationPath([{ id: 1, name: "Root" }]);
  }, []);

  const currentFolder: FolderInfo | null = folderContents?.folder || null;
  const subfolders: SubfolderItem[] = (folderContents?.subfolders || []).filter(folder => folder.id !== 1);
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
