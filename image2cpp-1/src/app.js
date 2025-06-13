document.addEventListener("DOMContentLoaded", function () {
  const imageInput = document.getElementById("imageInput");
  const convertButton = document.getElementById("convertButton");
  const outputSection = document.getElementById("outputSection");
  const output = document.getElementById("output");
  const imageInput2 = document.getElementById("imageInput2");
  const ditherButton = document.getElementById("ditherButton");
  const frameCountInput = document.getElementById("frameCount");
  const preview1 = document.getElementById("preview1");
  const preview2 = document.getElementById("preview2");
  const animPreview = document.getElementById("animPreview");

  imageInput.addEventListener("change", function () {
    if (imageInput.files[0]) {
      preview1.src = URL.createObjectURL(imageInput.files[0]);
    } else {
      preview1.src = "";
    }
  });
  imageInput2.addEventListener("change", function () {
    if (imageInput2.files[0]) {
      preview2.src = URL.createObjectURL(imageInput2.files[0]);
    } else {
      preview2.src = "";
    }
  });

  convertButton.addEventListener("click", function () {
    const file = imageInput.files[0];
    if (!file) {
      alert("Please select an image file.");
      return;
    }

    const reader = new FileReader();
    reader.onload = function (e) {
      const img = new Image();
      img.onload = function () {
        const width = img.width;
        const height = img.height;
        // Draw image to canvas
        const canvas = document.createElement("canvas");
        canvas.width = width;
        canvas.height = height;
        const ctx = canvas.getContext("2d");
        ctx.drawImage(img, 0, 0, width, height);
        const imageData = ctx.getImageData(0, 0, width, height).data;

        // Convert to 1-bit monochrome bitmap
        const bytesPerRow = Math.ceil(width / 8);
        const bitmap = new Uint8Array(bytesPerRow * height);
        for (let y = 0; y < height; y++) {
          for (let x = 0; x < width; x++) {
            const idx = (y * width + x) * 4;
            // Simple threshold: average RGB < 128 is black (1), else white (0)
            const avg =
              (imageData[idx] + imageData[idx + 1] + imageData[idx + 2]) / 3;
            const bit = avg < 128 ? 1 : 0;
            if (bit) {
              bitmap[y * bytesPerRow + (x >> 3)] |= 0x80 >> (x & 7);
            }
          }
        }

        // Format name for C variable
        const baseName = file.name.replace(/\.[^/.]+$/, "");
        const varName = `epd_bitmap_${baseName.replace(/[^a-zA-Z0-9_]/g, "_")}`;
        // Output as Arduino bitmap C array
        let cppArray = `// '${baseName}', ${width}x${height}px\nconst unsigned char ${varName} [] PROGMEM = {\n\t`;
        for (let i = 0; i < bitmap.length; i++) {
          cppArray += `0x${bitmap[i].toString(16).padStart(2, "0")}`;
          if (i < bitmap.length - 1) cppArray += ", ";
          if ((i + 1) % 16 === 0) cppArray += "\n\t";
        }
        cppArray += `\n};`;
        output.textContent = cppArray;
        outputSection.style.display = "block";
      };
      img.src = URL.createObjectURL(file);
    };
    reader.readAsDataURL(file);
  });

  function loadImage(file) {
    return new Promise((resolve, reject) => {
      const reader = new FileReader();
      reader.onload = function (e) {
        const img = new Image();
        img.onload = () => resolve(img);
        img.onerror = reject;
        img.src = e.target.result;
      };
      reader.readAsDataURL(file);
    });
  }

  ditherButton.addEventListener("click", async function () {
    const file1 = imageInput.files[0];
    const file2 = imageInput2.files[0];
    const frameCount = frameCountInput
      ? Math.max(2, parseInt(frameCountInput.value) || 10)
      : 10;
    if (!file1 || !file2) {
      alert("Please select two image files.");
      return;
    }
    try {
      const img1 = await loadImage(file1);
      const img2 = await loadImage(file2);
      const width = Math.min(img1.width, img2.width);
      const height = Math.min(img1.height, img2.height);

      // Draw both images to canvases, resize to match
      const canvas1 = document.createElement("canvas");
      const canvas2 = document.createElement("canvas");
      canvas1.width = canvas2.width = width;
      canvas1.height = canvas2.height = height;
      const ctx1 = canvas1.getContext("2d");
      const ctx2 = canvas2.getContext("2d");
      ctx1.drawImage(img1, 0, 0, width, height);
      ctx2.drawImage(img2, 0, 0, width, height);
      const data1 = ctx1.getImageData(0, 0, width, height).data;
      const data2 = ctx2.getImageData(0, 0, width, height).data;

      let cppArrays = "";
      let frameBitmaps = []; // For animation preview

      for (let f = 0; f < frameCount; f++) {
        const alpha = f / (frameCount - 1);
        // Blend each channel linearly
        const blended = new Uint8ClampedArray(width * height * 4);
        for (let i = 0; i < blended.length; i += 4) {
          blended[i] = Math.round(data1[i] * (1 - alpha) + data2[i] * alpha);
          blended[i + 1] = Math.round(
            data1[i + 1] * (1 - alpha) + data2[i + 1] * alpha
          );
          blended[i + 2] = Math.round(
            data1[i + 2] * (1 - alpha) + data2[i + 2] * alpha
          );
          blended[i + 3] = 255;
        }
        // Dither
        const gray = [];
        for (let i = 0; i < blended.length; i += 4) {
          gray.push((blended[i] + blended[i + 1] + blended[i + 2]) / 3);
        }
        const dithered = new Uint8Array(gray.length);
        for (let y = 0; y < height; y++) {
          for (let x = 0; x < width; x++) {
            const idx = y * width + x;
            let old = gray[idx];
            let newVal = old < 128 ? 0 : 255;
            dithered[idx] = newVal === 0 ? 1 : 0;
            let err = old - newVal;
            if (x + 1 < width) gray[idx + 1] += (err * 7) / 16;
            if (x > 0 && y + 1 < height)
              gray[idx + width - 1] += (err * 3) / 16;
            if (y + 1 < height) gray[idx + width] += (err * 5) / 16;
            if (x + 1 < width && y + 1 < height)
              gray[idx + width + 1] += (err * 1) / 16;
          }
        }
        // Pack bits into bytes
        const bytesPerRow = Math.ceil(width / 8);
        const bitmap = new Uint8Array(bytesPerRow * height);
        for (let y = 0; y < height; y++) {
          for (let x = 0; x < width; x++) {
            if (dithered[y * width + x]) {
              bitmap[y * bytesPerRow + (x >> 3)] |= 0x80 >> (x & 7);
            }
          }
        }
        // Output as Arduino bitmap C array
        const varName = `epd_bitmap_dithered_blend_${f}`;
        cppArrays += `// Dithered blend frame ${
          f + 1
        }/${frameCount}, ${width}x${height}px\nconst unsigned char ${varName} [] PROGMEM = {\n\t`;
        for (let i = 0; i < bitmap.length; i++) {
          cppArrays += `0x${bitmap[i].toString(16).padStart(2, "0")}`;
          if (i < bitmap.length - 1) cppArrays += ", ";
          if ((i + 1) % 16 === 0) cppArrays += "\n\t";
        }
        cppArrays += `\n};\n\n`;

        // Prepare frame for animation preview
        if (animPreview) {
          const ctx = animPreview.getContext("2d");
          const frameImage = ctx.createImageData(width, height);
          for (let i = 0; i < dithered.length; i++) {
            const v = dithered[i] ? 0 : 255;
            frameImage.data[i * 4 + 0] = v;
            frameImage.data[i * 4 + 1] = v;
            frameImage.data[i * 4 + 2] = v;
            frameImage.data[i * 4 + 3] = 255;
          }
          frameBitmaps.push(frameImage);
        }
      }
      output.textContent = cppArrays;
      outputSection.style.display = "block";

      // Animation preview
      if (animPreview && frameBitmaps.length > 0) {
        const ctx = animPreview.getContext("2d");
        ctx.clearRect(0, 0, animPreview.width, animPreview.height);
        const scale = Math.min(
          animPreview.width / frameBitmaps[0].width,
          animPreview.height / frameBitmaps[0].height,
          1
        );
        let frameIdx = 0;
        if (window._animPreviewTimer) clearInterval(window._animPreviewTimer);
        window._animPreviewTimer = setInterval(() => {
          ctx.clearRect(0, 0, animPreview.width, animPreview.height);
          ctx.save();
          ctx.imageSmoothingEnabled = false;
          ctx.scale(scale, scale);
          ctx.putImageData(frameBitmaps[frameIdx], 0, 0);
          ctx.restore();
          frameIdx = (frameIdx + 1) % frameBitmaps.length;
        }, 1000 / 12); // ~12 FPS
      }
    } catch (err) {
      alert("Error processing images: " + err);
    }
  });
});
