// API configuration and helper functions
// Default to the deployed Render backend if no env var is set
const API_BASE_URL =
  process.env.NEXT_PUBLIC_API_URL ||
  'https://image-compression-1-3f25.onrender.com'

export const getApiUrl = (endpoint) => {
  return `${API_BASE_URL}${endpoint}`
}

export const apiClient = {
  /**
   * Compress an image file
   * @param {File} file - The image file to compress
   * @returns {Promise<Object>} Compression result with job_id
   */
  async compressImage(file) {
    const formData = new FormData()
    formData.append('file', file)

    const response = await fetch(`${API_BASE_URL}/api/compression/compress`, {
      method: 'POST',
      body: formData,
    })

    if (!response.ok) {
      throw new Error('Compression failed')
    }

    return response.json()
  },

  /**
   * Get compression result by job ID
   * @param {string} jobId - The compression job ID
   * @returns {Promise<Object>} Compression result with metrics
   */
  async getCompressionResult(jobId) {
    const response = await fetch(`${API_BASE_URL}/api/compression/result/${jobId}`)

    if (!response.ok) {
      throw new Error('Failed to fetch compression result')
    }

    return response.json()
  },

  /**
   * Get compression metrics for a job
   * @param {string} jobId - The compression job ID
   * @returns {Promise<Object>} Metrics data
   */
  async getMetrics(jobId) {
    const response = await fetch(`${API_BASE_URL}/api/compression/metrics/${jobId}`)

    if (!response.ok) {
      throw new Error('Failed to fetch metrics')
    }

    return response.json()
  },

  /**
   * Get compression history
   * @param {number} limit - Number of recent compressions to fetch
   * @returns {Promise<Array>} List of recent compression jobs
   */
  async getHistory(limit = 10) {
    const response = await fetch(
      `${API_BASE_URL}/api/compression/history?limit=${limit}`
    )

    if (!response.ok) {
      throw new Error('Failed to fetch history')
    }

    return response.json()
  },

  /**
   * Get analytics/summary statistics
   * @returns {Promise<Object>} Analytics data
   */
  async getAnalytics() {
    const response = await fetch(`${API_BASE_URL}/api/analytics/summary`)

    if (!response.ok) {
      throw new Error('Failed to fetch analytics')
    }

    return response.json()
  },

  /**
   * Download compressed image
   * @param {string} jobId - The compression job ID
   */
  async downloadCompressed(jobId) {
    window.location.href = `${API_BASE_URL}/api/compression/download/${jobId}`
  },

  /**
   * Download original image
   * @param {string} jobId - The compression job ID
   */
  async downloadOriginal(jobId) {
    window.location.href = `${API_BASE_URL}/api/compression/download/${jobId}?type=original`
  },
}

export default apiClient
