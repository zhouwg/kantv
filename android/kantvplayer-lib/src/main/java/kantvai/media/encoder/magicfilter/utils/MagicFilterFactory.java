package kantvai.media.encoder.magicfilter.utils;

import kantvai.media.encoder.magicfilter.advanced.MagicAmaroFilter;
import kantvai.media.encoder.magicfilter.advanced.MagicAntiqueFilter;
import kantvai.media.encoder.magicfilter.advanced.MagicBeautyFilter;
import kantvai.media.encoder.magicfilter.advanced.MagicBlackCatFilter;
import kantvai.media.encoder.magicfilter.advanced.MagicBrannanFilter;
import kantvai.media.encoder.magicfilter.advanced.MagicBrooklynFilter;
import kantvai.media.encoder.magicfilter.advanced.MagicCalmFilter;
import kantvai.media.encoder.magicfilter.advanced.MagicCoolFilter;
import kantvai.media.encoder.magicfilter.advanced.MagicCrayonFilter;
import kantvai.media.encoder.magicfilter.advanced.MagicEarlyBirdFilter;
import kantvai.media.encoder.magicfilter.advanced.MagicEmeraldFilter;
import kantvai.media.encoder.magicfilter.advanced.MagicEvergreenFilter;
import kantvai.media.encoder.magicfilter.advanced.MagicFreudFilter;
import kantvai.media.encoder.magicfilter.advanced.MagicHealthyFilter;
import kantvai.media.encoder.magicfilter.advanced.MagicHefeFilter;
import kantvai.media.encoder.magicfilter.advanced.MagicHudsonFilter;
import kantvai.media.encoder.magicfilter.advanced.MagicImageAdjustFilter;
import kantvai.media.encoder.magicfilter.advanced.MagicInkwellFilter;
import kantvai.media.encoder.magicfilter.advanced.MagicKevinFilter;
import kantvai.media.encoder.magicfilter.advanced.MagicLatteFilter;
import kantvai.media.encoder.magicfilter.advanced.MagicLomoFilter;
import kantvai.media.encoder.magicfilter.advanced.MagicN1977Filter;
import kantvai.media.encoder.magicfilter.advanced.MagicNashvilleFilter;
import kantvai.media.encoder.magicfilter.advanced.MagicNostalgiaFilter;
import kantvai.media.encoder.magicfilter.advanced.MagicPixarFilter;
import kantvai.media.encoder.magicfilter.advanced.MagicRiseFilter;
import kantvai.media.encoder.magicfilter.advanced.MagicRomanceFilter;
import kantvai.media.encoder.magicfilter.advanced.MagicSakuraFilter;
import kantvai.media.encoder.magicfilter.advanced.MagicSierraFilter;
import kantvai.media.encoder.magicfilter.advanced.MagicSketchFilter;
import kantvai.media.encoder.magicfilter.advanced.MagicSkinWhitenFilter;
import kantvai.media.encoder.magicfilter.advanced.MagicSunriseFilter;
import kantvai.media.encoder.magicfilter.advanced.MagicSunsetFilter;
import kantvai.media.encoder.magicfilter.advanced.MagicSutroFilter;
import kantvai.media.encoder.magicfilter.advanced.MagicSweetsFilter;
import kantvai.media.encoder.magicfilter.advanced.MagicTenderFilter;
import kantvai.media.encoder.magicfilter.advanced.MagicToasterFilter;
import kantvai.media.encoder.magicfilter.advanced.MagicValenciaFilter;
import kantvai.media.encoder.magicfilter.advanced.MagicWaldenFilter;
import kantvai.media.encoder.magicfilter.advanced.MagicWarmFilter;
import kantvai.media.encoder.magicfilter.advanced.MagicWhiteCatFilter;
import kantvai.media.encoder.magicfilter.advanced.MagicXproIIFilter;
import kantvai.media.encoder.magicfilter.base.MagicLookupFilter;
import kantvai.media.encoder.magicfilter.base.gpuimage.GPUImageBrightnessFilter;
import kantvai.media.encoder.magicfilter.base.gpuimage.GPUImageContrastFilter;
import kantvai.media.encoder.magicfilter.base.gpuimage.GPUImageExposureFilter;
import kantvai.media.encoder.magicfilter.base.gpuimage.GPUImageFilter;
import kantvai.media.encoder.magicfilter.base.gpuimage.GPUImageHueFilter;
import kantvai.media.encoder.magicfilter.base.gpuimage.GPUImageSaturationFilter;
import kantvai.media.encoder.magicfilter.base.gpuimage.GPUImageSharpenFilter;

public class MagicFilterFactory {

    public static GPUImageFilter initFilters(MagicFilterType type) {
        switch (type) {
            case NONE:
                return new GPUImageFilter();
            case WHITECAT:
                return new MagicWhiteCatFilter();
            case BLACKCAT:
                return new MagicBlackCatFilter();
            case SKINWHITEN:
                return new MagicSkinWhitenFilter();
            case BEAUTY:
                return new MagicBeautyFilter();
            case ROMANCE:
                return new MagicRomanceFilter();
            case SAKURA:
                return new MagicSakuraFilter();
            case AMARO:
                return new MagicAmaroFilter();
            case WALDEN:
                return new MagicWaldenFilter();
            case ANTIQUE:
                return new MagicAntiqueFilter();
            case CALM:
                return new MagicCalmFilter();
            case BRANNAN:
                return new MagicBrannanFilter();
            case BROOKLYN:
                return new MagicBrooklynFilter();
            case EARLYBIRD:
                return new MagicEarlyBirdFilter();
            case FREUD:
                return new MagicFreudFilter();
            case HEFE:
                return new MagicHefeFilter();
            case HUDSON:
                return new MagicHudsonFilter();
            case INKWELL:
                return new MagicInkwellFilter();
            case KEVIN:
                return new MagicKevinFilter();
            case LOCKUP:
                return new MagicLookupFilter("");
            case LOMO:
                return new MagicLomoFilter();
            case N1977:
                return new MagicN1977Filter();
            case NASHVILLE:
                return new MagicNashvilleFilter();
            case PIXAR:
                return new MagicPixarFilter();
            case RISE:
                return new MagicRiseFilter();
            case SIERRA:
                return new MagicSierraFilter();
            case SUTRO:
                return new MagicSutroFilter();
            case TOASTER2:
                return new MagicToasterFilter();
            case VALENCIA:
                return new MagicValenciaFilter();
            case XPROII:
                return new MagicXproIIFilter();
            case EVERGREEN:
                return new MagicEvergreenFilter();
            case HEALTHY:
                return new MagicHealthyFilter();
            case COOL:
                return new MagicCoolFilter();
            case EMERALD:
                return new MagicEmeraldFilter();
            case LATTE:
                return new MagicLatteFilter();
            case WARM:
                return new MagicWarmFilter();
            case TENDER:
                return new MagicTenderFilter();
            case SWEETS:
                return new MagicSweetsFilter();
            case NOSTALGIA:
                return new MagicNostalgiaFilter();
            case SUNRISE:
                return new MagicSunriseFilter();
            case SUNSET:
                return new MagicSunsetFilter();
            case CRAYON:
                return new MagicCrayonFilter();
            case SKETCH:
                return new MagicSketchFilter();
            //image adjust
            case BRIGHTNESS:
                return new GPUImageBrightnessFilter();
            case CONTRAST:
                return new GPUImageContrastFilter();
            case EXPOSURE:
                return new GPUImageExposureFilter();
            case HUE:
                return new GPUImageHueFilter();
            case SATURATION:
                return new GPUImageSaturationFilter();
            case SHARPEN:
                return new GPUImageSharpenFilter();
            case IMAGE_ADJUST:
                return new MagicImageAdjustFilter();
            default:
                return null;
        }
    }
}
