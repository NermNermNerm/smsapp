#include "downscale.h"

QUrl downscaleImage(const QUrl& sourceUrl)
{
    // Convert URL to a standard local file path
    QString inputPath = sourceUrl.toLocalFile();
    if (inputPath.isEmpty()) {
        return sourceUrl; // Pass-through if it's not a local file URL
    }

    // 600 KB = 614,400 bytes. We use 580 KB as a safe cushion.
    const qint64 maxBytes = 580 * 1024;

    // OPTIMIZATION: If the file is already small enough, leave it alone!
    QFileInfo fileInfo(inputPath);
    if (fileInfo.exists() && fileInfo.size() <= maxBytes) {
        return sourceUrl;
    }

    // Try to load the file as an image
    QImage sourceImage(inputPath);
    if (sourceImage.isNull()) {
        // If it's a PDF, Zip, or text file, QImage will fail to load it.
        // We pass the original URL through so the rest of your app pipeline doesn't break.
        return sourceUrl;
    }

    int quality = 85; // Start with a crisp JPEG quality
    QImage workingImage = sourceImage;
    QByteArray compressedData;

    // The Optimization Loop
    while (true) {
        compressedData.clear();
        QBuffer buffer(&compressedData);
        buffer.open(QIODevice::WriteOnly);

        // Compress to a JPEG in memory
        if (!workingImage.save(&buffer, "JPG", quality)) {
            qWarning() << "Failed to compress image in memory buffer.";
            return sourceUrl; // Fallback to original on encoding failure
        }

        // Success condition: We squeezed it under the 600KB threshold
        if (compressedData.size() <= maxBytes) {
            break;
        }

        // Step 1: Lower the JPEG quality until we hit a floor of 50
        if (quality > 50) {
            quality -= 10;
        }
        // Step 2: If quality tuning isn't enough, slice 20% off the dimensions
        else {
            // Guard to stop shrinking if the image gets microscopic
            if (workingImage.width() < 400 || workingImage.height() < 400) {
                break;
            }

            // Smooth scaling keeps text legible in screenshots
            workingImage = workingImage.scaled(
                workingImage.size() * 0.8,
                Qt::KeepAspectRatio,
                Qt::SmoothTransformation
                );

            quality = 85; // Reset quality for the newly resized resolution
        }
    }

    // Write the optimized data out to a new unique temporary file
    QString tempTemplate = QDir::tempPath() + "/downscaled_image_XXXXXX.jpg";
    QTemporaryFile tempFile(tempTemplate);
    tempFile.setAutoRemove(false); // Let the OS temp cleaner sweep it up later

    if (!tempFile.open()) {
        qWarning() << "Failed to create temporary output file.";
        return sourceUrl;
    }

    tempFile.write(compressedData);
    QString outputPath = tempFile.fileName();
    tempFile.close();

    // Return the fresh local file:// URL
    return QUrl::fromLocalFile(outputPath);
}
