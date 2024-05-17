package cdeos.media.encoder.magicfilter.utils;

import cdeos.media.encoder.magicfilter.advanced.MagicAmaroFilter;
import cdeos.media.encoder.magicfilter.advanced.MagicAntiqueFilter;
import cdeos.media.encoder.magicfilter.advanced.MagicBeautyFilter;
import cdeos.media.encoder.magicfilter.advanced.MagicBlackCatFilter;
import cdeos.media.encoder.magicfilter.advanced.MagicBrannanFilter;
import cdeos.media.encoder.magicfilter.advanced.MagicBrooklynFilter;
import cdeos.media.encoder.magicfilter.advanced.MagicCalmFilter;
import cdeos.media.encoder.magicfilter.advanced.MagicCoolFilter;
import cdeos.media.encoder.magicfilter.advanced.MagicCrayonFilter;
import cdeos.media.encoder.magicfilter.advanced.MagicEarlyBirdFilter;
import cdeos.media.encoder.magicfilter.advanced.MagicEmeraldFilter;
import cdeos.media.encoder.magicfilter.advanced.MagicEvergreenFilter;
import cdeos.media.encoder.magicfilter.advanced.MagicFreudFilter;
import cdeos.media.encoder.magicfilter.advanced.MagicHealthyFilter;
import cdeos.media.encoder.magicfilter.advanced.MagicHefeFilter;
import cdeos.media.encoder.magicfilter.advanced.MagicHudsonFilter;
import cdeos.media.encoder.magicfilter.advanced.MagicImageAdjustFilter;
import cdeos.media.encoder.magicfilter.advanced.MagicInkwellFilter;
import cdeos.media.encoder.magicfilter.advanced.MagicKevinFilter;
import cdeos.media.encoder.magicfilter.advanced.MagicLatteFilter;
import cdeos.media.encoder.magicfilter.advanced.MagicLomoFilter;
import cdeos.media.encoder.magicfilter.advanced.MagicN1977Filter;
import cdeos.media.encoder.magicfilter.advanced.MagicNashvilleFilter;
import cdeos.media.encoder.magicfilter.advanced.MagicNostalgiaFilter;
import cdeos.media.encoder.magicfilter.advanced.MagicPixarFilter;
import cdeos.media.encoder.magicfilter.advanced.MagicRiseFilter;
import cdeos.media.encoder.magicfilter.advanced.MagicRomanceFilter;
import cdeos.media.encoder.magicfilter.advanced.MagicSakuraFilter;
import cdeos.media.encoder.magicfilter.advanced.MagicSierraFilter;
import cdeos.media.encoder.magicfilter.advanced.MagicSketchFilter;
import cdeos.media.encoder.magicfilter.advanced.MagicSkinWhitenFilter;
import cdeos.media.encoder.magicfilter.advanced.MagicSunriseFilter;
import cdeos.media.encoder.magicfilter.advanced.MagicSunsetFilter;
import cdeos.media.encoder.magicfilter.advanced.MagicSutroFilter;
import cdeos.media.encoder.magicfilter.advanced.MagicSweetsFilter;
import cdeos.media.encoder.magicfilter.advanced.MagicTenderFilter;
import cdeos.media.encoder.magicfilter.advanced.MagicToasterFilter;
import cdeos.media.encoder.magicfilter.advanced.MagicValenciaFilter;
import cdeos.media.encoder.magicfilter.advanced.MagicWaldenFilter;
import cdeos.media.encoder.magicfilter.advanced.MagicWarmFilter;
import cdeos.media.encoder.magicfilter.advanced.MagicWhiteCatFilter;
import cdeos.media.encoder.magicfilter.advanced.MagicXproIIFilter;
import cdeos.media.encoder.magicfilter.base.MagicLookupFilter;
import cdeos.media.encoder.magicfilter.base.gpuimage.GPUImageBrightnessFilter;
import cdeos.media.encoder.magicfilter.base.gpuimage.GPUImageContrastFilter;
import cdeos.media.encoder.magicfilter.base.gpuimage.GPUImageExposureFilter;
import cdeos.media.encoder.magicfilter.base.gpuimage.GPUImageFilter;
import cdeos.media.encoder.magicfilter.base.gpuimage.GPUImageHueFilter;
import cdeos.media.encoder.magicfilter.base.gpuimage.GPUImageSaturationFilter;
import cdeos.media.encoder.magicfilter.base.gpuimage.GPUImageSharpenFilter;

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
