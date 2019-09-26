classdef CallbackData < uint16
   enumeration
      None	       (0) % only get the callback but no data delivery
      Raw          (1) % raw frames, if exclusively used no processing pipe is executed (no calibration data is needed)
      Depth        (2) % one depth and grayscale image will be delivered for the complete sequence
      Intermediate (4) % all intermediate data will be delivered which are generated in the processing pipeline
   end
end