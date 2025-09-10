export interface ErrorMessagesProps {
  uploadError?: Error | null;
  createError?: Error | null;
  contentsError?: Error | null;
}

const ErrorMessages = ({ uploadError, createError, contentsError }: ErrorMessagesProps) => {
  const hasErrors = uploadError || createError || contentsError;

  if (!hasErrors) {
    return null;
  }

  return (
    <div className="error-message">
      {uploadError && <p>Upload error: {uploadError.message}</p>}
      {createError && <p>Create folder error: {createError.message}</p>}
      {contentsError && <p>Failed to load folder contents: {contentsError.message}</p>}
    </div>
  );
};

export default ErrorMessages;
