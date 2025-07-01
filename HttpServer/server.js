// server.js
const express = require('express');
const multer  = require('multer');
const path = require('path');
const fs = require('fs');
const os = require('os');
const hostname = os.hostname();

const uploadDir = path.join(__dirname, 'gbaSaveFiles');
const storage = multer.diskStorage({
  destination: (req, file, cb) => cb(null, uploadDir),
  filename: (req, file, cb) => cb(null, file.originalname)
});
const upload = multer({ storage });
const app = express();

app.use(express.raw({ type: 'application/octet-stream', limit: '10mb' }));

app.use((req, res, next) => {
  console.log(`[${new Date().toISOString()}] ${req.method} ${req.url}`);
  next();
});


// Upload files to gbaSaveFiles folder
app.post('/gbaSaveFiles', upload.single('file'), (req, res) => {
  console.log("FILE UPLOAD ENDPOINT CALLED")
  res.send('File uploaded\n');
});


// List all files with timestamps, human-readable dates, and file size
app.get('/gbaSaveFiles', (req, res) => {
  console.log("SAVE FILES REQUESTED");

  fs.readdir(uploadDir, (err, files) => {
    if (err) {
      return res.status(500).send('Error reading directory');
    }

    const fileData = files.map(filename => {
      const filePath = path.join(uploadDir, filename);
      const stats = fs.statSync(filePath);
      return {
        filename,
        timeStamp: stats.mtimeMs,
        lastModified: new Date(stats.mtimeMs).toISOString(),
        size: stats.size // in bytes
      };
    });

    res.json(fileData);
  });
});

// Download a file in the gbaSaveFiles folder
app.get('/gbaSaveFiles/:filename', (req, res) => {
  const filename = req.params.filename;
  const filePath = path.join(uploadDir, filename);
  if (fs.existsSync(filePath)) {
    res.download(filePath);
  } else {
    // console.log("File not found:", filePath);
    res.status(404).send('File not found');
  }
});

app.listen(3000, () => {
  console.log(`Upload server listening on http://${hostname}:3000`);
  console.log(`Upload server path is ${uploadDir}`);
});

