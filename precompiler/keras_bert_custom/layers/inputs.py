from keras_bert.backend import keras


def get_inputs(seq_len, batch_size=None):
    """Get input layers.

    See: https://arxiv.org/pdf/1810.04805.pdf

    :param seq_len: Length of the sequence or None.
    """
    names = ['Token', 'Segment', 'Masked']
    return [keras.layers.Input(
        batch_shape=(batch_size,seq_len),
        name='Input-%s' % name,
    ) for name in names]
