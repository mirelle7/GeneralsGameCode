/*
**	Command & Conquer Generals Zero Hour(tm)
**	Copyright 2026 TheSuperHackers
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

// Quick Look thumbnail extension for World Builder maps (Finder icons, Quick Look, Spotlight).

#import <CoreGraphics/CoreGraphics.h>
#import <Foundation/Foundation.h>
#import <QuickLookThumbnailing/QuickLookThumbnailing.h>

#include "MapThumbnail/MapThumbnail.h"

#include <algorithm>
#include <cmath>
#include <memory>

@interface MapThumbnailProvider : QLThumbnailProvider
@end

@implementation MapThumbnailProvider

- (void)provideThumbnailForFileRequest:(QLFileThumbnailRequest *)request
					completionHandler:(void (^)(QLThumbnailReply *_Nullable, NSError *_Nullable))handler
{
	const CGFloat scale = request.scale > 0 ? request.scale : 1;
	const int maxPixels = int(std::ceil(std::max(request.maximumSize.width, request.maximumSize.height) * scale));

	const MapThumbnail::Settings settings = MapThumbnail::loadSettings();
	std::shared_ptr<MapThumbnail::Image> image = std::make_shared<MapThumbnail::Image>();
	std::string error;
	if (!MapThumbnail::renderFile(request.fileURL.fileSystemRepresentation, maxPixels, settings, *image, &error))
	{
		NSDictionary *info = @{ NSLocalizedDescriptionKey : [NSString stringWithUTF8String:error.c_str()] };
		handler(nil, [NSError errorWithDomain:@"MapThumbnail" code:1 userInfo:info]);
		return;
	}

	const CGSize size = CGSizeMake(image->width / scale, image->height / scale);
	handler([QLThumbnailReply replyWithContextSize:size
									  drawingBlock:^BOOL(CGContextRef context) {
		CGColorSpaceRef space = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
		CGDataProviderRef data = CGDataProviderCreateWithData(nullptr, image->rgba.data(), image->rgba.size(), nullptr);
		CGImageRef cgImage = CGImageCreate(image->width, image->height, 8, 32, size_t(image->width) * 4, space,
			kCGBitmapByteOrderDefault | kCGImageAlphaNoneSkipLast, data, nullptr, true, kCGRenderingIntentDefault);
		if (cgImage)
			CGContextDrawImage(context, CGRectMake(0, 0, size.width, size.height), cgImage);
		CGImageRelease(cgImage);
		CGDataProviderRelease(data);
		CGColorSpaceRelease(space);
		return cgImage != nullptr;
	}],
		nil);
}

@end
