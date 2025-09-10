export interface NavigationItem {
  id: number | null;
  name: string;
}

export interface NavigationBreadcrumbProps {
  navigationPath: NavigationItem[];
  navigateToIndexInPath: (index: number) => void;
}

const NavigationBreadcrumb = ({ navigationPath, navigateToIndexInPath }: NavigationBreadcrumbProps) => {
  return (
    <nav className="breadcrumb">
      {navigationPath.map((item, index) => (
        <span key={`${item.id}-${index}`}>
          <button 
            className="breadcrumb-item" 
            onClick={() => navigateToIndexInPath(index)} 
            disabled={index === navigationPath.length - 1}
          >
            {item.name}
          </button>
          {index < navigationPath.length - 1 && <span className="breadcrumb-separator">›</span>}
        </span>
      ))}
    </nav>
  );
};

export default NavigationBreadcrumb;