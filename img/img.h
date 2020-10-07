#ifndef __IMG_H_
#define __IMG_H_

#include <utils.h>

#include <jpeglib.h>
#include <setjmp.h>

#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include <string.h>
extern "C" {
    /* FIXME: Export std_huff_tables somehow from .so */
#define MEMZERO(target, size) \
  bzero((void *)(target), (size_t)(size))
#define MEMCOPY(dest, src, size) \
  bcopy((const void *)(src), (void *)(dest), (size_t)(size))
#define ERREXIT(a, b) {}
    const char *JERR_BAD_HUFF_TABLE = "Bogus Huffman table definition";
#include "jstdhuff.c"
}

void jpeg_output_message(j_common_ptr cinfo) {
  char buffer[JMSG_LENGTH_MAX];

  /* Create the message */
  (*cinfo->err->format_message) (cinfo, buffer);

  /* Send it to stderr, adding a newline */
  DBG_STREAM(jpeg) << "JPEG error: " << buffer << ENDL;
  //throw std::runtime_error(((std::string)"JPEG error: ") + buffer);
  //// no leaks ????
}

void jpeg_error_exit(j_common_ptr cinfo) {
  /* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
  auto myerr = cinfo->err;

  char buffer[JMSG_LENGTH_MAX];
  (*cinfo->err->format_message) (cinfo, buffer);

  throw std::runtime_error(((std::string)"JPEG error: ") + buffer);
}

class JPEGChannelProfile: public jpeg_component_info {
    public:

    JPEGChannelProfile() {
        /* default settings */
        h_samp_factor = 1;
        v_samp_factor = 1;
        quant_tbl_no = 0;
        dc_tbl_no = 0;
        ac_tbl_no = 0;
    }
};

class BestProfile: public JPEGChannelProfile {
    public:

    BestProfile() {
        h_samp_factor = 2;
        v_samp_factor = 2;
    }
};

class MediumProfile: public JPEGChannelProfile {
    public:

    MediumProfile() {
    }
};

class LowProfile: public JPEGChannelProfile {
    public:

    LowProfile() {
        quant_tbl_no = 1;
        dc_tbl_no = 1;
        ac_tbl_no = 1;
    }
};

template<const int component_count>
class JPEGCompressor : public jpeg_compress_struct {
    inline void _setup_channel(jpeg_component_info *comp_ptr) {
        return;
    }

    template<class T1, class... Tsetup>
    inline void _setup_channel(jpeg_component_info *comp_ptr, T1 &c, Tsetup&... rest) {
        *comp_ptr = c;
        _setup_channel(comp_ptr+1, rest...);
    }

    void setup_defaults() {
      /* Allocate comp_info array large enough for maximum component count.
       * Array is made permanent in case application wants to compress
       * multiple images at same param settings.
       */
      comp_info = NULL;

      /* Initialize everything not dependent on the color space */

      scale_num = 1;         /* 1:1 scaling */
      scale_denom = 1;

      data_precision = BITS_IN_JSAMPLE;
      /* Set up two quantization tables using default quality of 75 */
      jpeg_set_quality(this, 75, TRUE);
      /* Set up two Huffman tables */
      std_huff_tables((j_common_ptr)this);

      /* Initialize default arithmetic coding conditioning */
      for (int i = 0; i < NUM_ARITH_TBLS; i++) {
        arith_dc_L[i] = 0;
        arith_dc_U[i] = 1;
        arith_ac_K[i] = 5;
      }

      /* Default is no multiple-scan output */
      scan_info = NULL;
      num_scans = 0;

      /* Expect normal source image, not raw downsampled data */
      raw_data_in = FALSE;

      /* Use Huffman coding, not arithmetic coding, by default */
      arith_code = FALSE;

      /* By default, don't do extra passes to optimize entropy coding */
      /* The standard Huffman tables are only valid for 8-bit data precision.
       * If the precision is higher, force optimization on so that usable
       * tables will be computed.  This test can be removed if default tables
       * are supplied that are valid for the desired precision.
       */
      optimize_coding = (data_precision > 8);

      /* By default, use the simpler non-cosited sampling alignment */
      CCIR601_sampling = FALSE;

      /* By default, apply fancy downsampling */
      do_fancy_downsampling = TRUE;

      /* No input smoothing */
      smoothing_factor = 0;

      /* DCT algorithm preference */
      dct_method = JDCT_FLOAT;

      /* No restart markers */
      restart_interval = 0;
      restart_in_rows = 0;

      /* Fill in default JFIF marker parameters.  Note that whether the marker
       * will actually be written is determined by jpeg_set_colorspace.
       *
       * By default, the library emits JFIF version code 1.01.
       * An application that wants to emit JFIF 1.02 extension markers should set
       * JFIF_minor_version to 2.  We could probably get away with just defaulting
       * to 1.02, but there may still be some decoders in use that will complain
       * about that; saying 1.01 should minimize compatibility problems.
       */
      JFIF_major_version = 1; /* 1.02 */
      JFIF_minor_version = 2;
      density_unit = 0;      /* Pixel size is unknown by default */
      X_density = 1;         /* Pixel aspect ratio is square by default */
      Y_density = 1;
    }

    void do_compress(const cv::Mat &im) {
        if (unlikely(im.depth() != CV_8U)) {
            throw std::runtime_error("JPEG canc ompress only 8-bit unsigned data");
        }
        if (unlikely(im.channels() != component_count)) {
            throw std::runtime_error(((std::string)"Passed image with ") + std::to_string(im.channels()) + " channels, but " + std::to_string(component_count) + " required");
        }

        /* set image size */
        image_width = im.cols;
        image_height = im.rows;
        jpeg_start_compress(this, TRUE);

        auto image_buffer = im.ptr(); // TODO: exceptions ?
        auto row_stride = image_width * component_count; /* JSAMPLEs per row in image_buffer */
        while (next_scanline < image_height) {
            const unsigned char *row_pointer[1];
            row_pointer[0] = &image_buffer[next_scanline * row_stride];
            jpeg_write_scanlines(this, (unsigned char**)row_pointer, 1); // TODO: more at once ?
        }

        jpeg_finish_compress(this);
    }

    public:
    struct jpeg_error_mgr jerr;
    jpeg_component_info component_settings[component_count];

    template<class... Targs>
    void setup_channels(const Targs... cs) {
        constexpr int n = sizeof...(cs);
        if constexpr (n != component_count) {
            throw std::runtime_error(((std::string)"Selected ") + std::to_string(component_count) + " channels, but " + std::to_string(n) + " passed to setup");
        }

        num_components = component_count;
        input_components = component_count;
        comp_info = component_settings;

        for (int i=0; i<component_count; i++) {
            comp_info[i].component_id = i+1;
        }
        _setup_channel(component_settings, cs...);
    }

    JPEGCompressor() {
        err = jpeg_std_error(&jerr);
        err->output_message = jpeg_output_message;
        err->error_exit = jpeg_error_exit;
        jpeg_create_compress(this);
        setup_defaults();
    }

#ifdef OPENCV_CORE_BASE_HPP
    void write_to_file(const char *fname, const cv::Mat &im) {
        FILE *outfile = fopen(fname, "wb");
        if (unlikely(outfile == NULL)) {
            throw std::runtime_error(((std::string)"Opening file \"") + fname + "\" failed.");
        }

        try {
            jpeg_stdio_dest(this, outfile);
            do_compress(im);
        } catch(...) {
            fclose(outfile); // FIXME: better file class ?
            throw;
        }
        fclose(outfile);
    }
#endif /* OPENCV_CORE_BASE_HPP */

    ~JPEGCompressor() {
        jpeg_destroy_compress(this);
    }
};

class RGBCompressor : public JPEGCompressor<3> {
    public:

    RGBCompressor() {
        setup_channels(
            MediumProfile(),
            MediumProfile(),
            MediumProfile()
        );
    }
};

class RGBACompressor : public JPEGCompressor<4> {
    public:

    RGBACompressor() {
        setup_channels(
            MediumProfile(),
            MediumProfile(),
            MediumProfile(),
            LowProfile()
        );
    }
};

class YCbCrCompressor : public JPEGCompressor<3> {
    public:

    YCbCrCompressor() {
        setup_channels(
            BestProfile(),
            LowProfile(),
            LowProfile()
        );
    }
};

class YCbCrACompressor : public JPEGCompressor<4> {
    public:

    YCbCrACompressor() {
        setup_channels(
            BestProfile(),
            LowProfile(),
            LowProfile(),
            LowProfile()
        );
    }
};

class NormalCompressor : public JPEGCompressor<2> {
    public:

    NormalCompressor() {
        /* orthogonal vectors length */
        setup_channels(
            MediumProfile(),
            MediumProfile()
        );
    }
};

//template<class T>
void write_img() {
    //RGBCompressor compr;
    //YCbCrACompressor compr;
    //RGBACompressor compr;
    NormalCompressor compr;
    cv::Mat im = cv::Mat::zeros(cv::Size(128, 256), CV_8UC2);
    DBG_STREAM(jpeg) << im.cols << " " << im.rows << " " << im.channels() << ENDL;
    compr.write_to_file("a.jpg", im);
    compr.write_to_file("b.jpg", im);
}

class JPEGDecompressor : public jpeg_decompress_struct{
    void finish_decompress(unsigned char *image_buffer) {
        jpeg_start_decompress(this);

        auto row_stride = output_width * output_components;
        while (output_scanline < output_height) {
            unsigned char *row_pointer[1];
            row_pointer[0] = &image_buffer[output_scanline * row_stride];
            jpeg_read_scanlines(this, row_pointer, 1);
        }

        /* Step 7: Finish decompression */
        jpeg_finish_decompress(this);
    }

    public:

    jpeg_error_mgr jerr;

    JPEGDecompressor() {
        err = jpeg_std_error(&jerr);
        err->output_message = jpeg_output_message;
        err->error_exit = jpeg_error_exit;

        jpeg_create_decompress(this);
    }

    cv::Mat read_from_file_cv(const char *fname) {
        FILE *infile = fopen(fname, "rb");
        if (unlikely(infile == NULL)) {
            throw std::runtime_error(((std::string)"Opening file \"") + fname + "\" failed.");
        }

        cv::Mat r;

        try {
            jpeg_stdio_src(this, infile);
            jpeg_read_header(this, TRUE);

            r = cv::Mat(image_height, image_width, CV_MAKETYPE(CV_8U, num_components));

            finish_decompress(r.ptr());
        } catch(...) {
            fclose(infile);
            throw;
        }

        fclose(infile);
        return r;
    }

    ~JPEGDecompressor() {
        jpeg_destroy_decompress(this);
    }
};

void read_img() {
    JPEGDecompressor c;
    auto im = c.read_from_file_cv("a.jpg");
    DBG_STREAM(jpeg) << im.cols << " " << im.rows << " " << im.channels() << ENDL;
}

#endif /* __IMG_H_ */

