export interface UploadProgressProps {
  uploadProgress: number;
}

const UploadProgress = ({ uploadProgress }: UploadProgressProps) => {
  return (
    <div className="upload-progress">
      <div className="progress-bar">
        <div className="progress-fill" style={{ width: `${uploadProgress}%` }}></div>
      </div>
      <span>Uploading... {uploadProgress}%</span>
    </div>
  );
};

export default UploadProgress;
