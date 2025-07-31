# multi part form

- type of http content type that allows for requests to upload binary data, blobs and text
- good use case for this project because i want to upload data and tag it with metadata for searching etc.

# cors preflight

- normally javascript code blocks requests to a diff origin
- before allowing a cross origin request, browser will send a preflight request
  - OPTIONS type that includes the origin url
  - we return a response with the CORS headers
  - browser checks if the CORS headers matches what it needs. if it does, send the actual request
  - if the headers are missing or incorrect, the request is cancelled by the browser
  - all enforced by the browser, not the server
- in node when using something like express, can just do `app.use(cors());`
  - in c++, gotta do it manually (lol)
