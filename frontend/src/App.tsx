import { useState } from 'react'
import './App.css'

interface UploadedFile {
  id: string;
  filename: string;
  content_type: string;
  size: string;
}

interface UploadResponse {
  files: UploadedFile[];
}

interface ErrorResponse {
  error: string;
}

function App() {
  const [selectedFiles, setSelectedFiles] = useState<FileList | null>(null)
  const [uploading, setUploading] = useState(false)
  const [uploadResult, setUploadResult] = useState<UploadResponse | null>(null)
  const [error, setError] = useState<string | null>(null)

  const handleFileSelect = (event: React.ChangeEvent<HTMLInputElement>) => {
    setSelectedFiles(event.target.files)
    setUploadResult(null)
    setError(null)
  }

  const handleUpload = async () => {
    if (!selectedFiles || selectedFiles.length === 0) {
      setError('Please select at least one file')
      return
    }

    setUploading(true)
    setError(null)

    try {
      const formData = new FormData()
      
      // Add all selected files to form data
      for (let i = 0; i < selectedFiles.length; i++) {
        formData.append('file', selectedFiles[i])
      }

      const response = await fetch('/upload', {
        method: 'POST',
        body: formData,
      })

      if (response.ok) {
        const result: UploadResponse = await response.json()
        setUploadResult(result)
      } else {
        const errorResult: ErrorResponse = await response.json()
        setError(errorResult.error || 'Upload failed')
      }
    } catch (err) {
      setError('Network error: ' + (err instanceof Error ? err.message : 'Unknown error'))
    } finally {
      setUploading(false)
    }
  }

  const clearResults = () => {
    setSelectedFiles(null)
    setUploadResult(null)
    setError(null)
    // Reset the file input
    const fileInput = document.getElementById('file-input') as HTMLInputElement
    if (fileInput) {
      fileInput.value = ''
    }
  }

  return (
    <div className="app">
      <header>
        <h1>ByteBucket File Upload</h1>
        <p>Upload your files to ByteBucket storage</p>
      </header>

      <main>
        <div className="upload-section">
          <div className="file-input-container">
            <input
              id="file-input"
              type="file"
              multiple
              onChange={handleFileSelect}
              disabled={uploading}
              className="file-input"
            />
            <label htmlFor="file-input" className="file-input-label">
              {selectedFiles && selectedFiles.length > 0
                ? `${selectedFiles.length} file(s) selected`
                : 'Choose files to upload'}
            </label>
          </div>

          {selectedFiles && selectedFiles.length > 0 && (
            <div className="selected-files">
              <h3>Selected Files:</h3>
              <ul>
                {Array.from(selectedFiles).map((file, index) => (
                  <li key={index}>
                    {file.name} ({(file.size / 1024).toFixed(1)} KB)
                  </li>
                ))}
              </ul>
            </div>
          )}

          <div className="upload-actions">
            <button
              onClick={handleUpload}
              disabled={!selectedFiles || selectedFiles.length === 0 || uploading}
              className="upload-button"
            >
              {uploading ? 'Uploading...' : 'Upload Files'}
            </button>
            
            {(uploadResult || error) && (
              <button onClick={clearResults} className="clear-button">
                Clear Results
              </button>
            )}
          </div>
        </div>

        {error && (
          <div className="error-message">
            <h3>Error:</h3>
            <p>{error}</p>
          </div>
        )}

        {uploadResult && (
          <div className="success-message">
            <h3>Upload Successful!</h3>
            <div className="uploaded-files">
              {uploadResult.files.map((file, index) => (
                <div key={index} className="uploaded-file">
                  <h4>{file.filename}</h4>
                  <p><strong>File ID:</strong> {file.id}</p>
                  <p><strong>Type:</strong> {file.content_type}</p>
                  <p><strong>Size:</strong> {file.size} bytes</p>
                  <p><strong>Download URL:</strong> <code>/download/{file.id}</code></p>
                </div>
              ))}
            </div>
          </div>
        )}
      </main>
    </div>
  )
}

export default App
