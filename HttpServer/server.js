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

// app.post('/gbaSaveFiles', upload.single('file'), (req, res) => {
//   console.log("FILE UPLOAD ENDPOINT CALLED")
//   res.send('File uploaded');
// });



app.post('/gbaSaveFiles', (req, res) => {
  console.log("FILE UPLOAD ENDPOINT CALLED");

  if (!req.body || !(req.body instanceof Buffer)) {
    console.log("No raw body buffer received!");
    return res.status(400).send("Expected raw binary data");
  }

  console.log("Received file data length:", req.body.length);
  console.log(req.body.slice(0, 32).toString('hex'));  // print first 32 bytes in hex for sanity

  // Optionally save received data to verify it arrived correctly
  fs.writeFileSync('uploaded_file.bin', req.body);

  res.send('File uploaded');
});

// app.post('/gbaSaveFiles', upload.single('file'), (req, res) => {
//   console.log("FILE UPLOAD ENDPOINT CALLED");

//   if (!req.file) {
//     console.error("No file received!");
//     return res.status(400).send("No file received");
//   }

//   console.log("Received file:");
//   console.log("  originalname:", req.file.originalname);
//   console.log("  mimetype:", req.file.mimetype);
//   console.log("  size:", req.file.size);
//   console.log("  buffer:", req.file.buffer.toString());

//   // Optionally save the file to disk:
//   const fs = require('fs');
//   fs.writeFileSync(`./uploads/${req.file.originalname || 'upload.bin'}`, req.file.buffer);

//   res.send('File uploaded successfully');
// });


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

// Download specific file
// app.get('/gbaSaveFiles/:filename', (req, res) => {
//   console.log("Request for file:", filename);
//   const filePath = path.join(uploadDir, req.params.filename);
//   if (fs.existsSync(filePath)) {
//     res.download(filePath);
//   } else {
//     res.status(404).send('File not found');
//   }
// });
app.get('/gbaSaveFiles/:filename', (req, res) => {
  const filename = req.params.filename;
  // console.log("Request for file:", filename);

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
  // console.log(`MUNYAH!!!`);
});

