// API configuration and helper functions
const API_BASE_URL = process.env.NEXT_PUBLIC_API_URL || 'http://localhost:8000/api'

export const apiClient = {
  /**
   * Compress an image file
   * @param {File} file - The image file to compress
   * @returns {Promise<Object>} Compression result with job_id
   */
  async compressImage(file) {
    const formData = new FormData()
    formData.append('file', file)

    const response = await fetch(`${API_BASE_URL}/compression/compress`, {
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
    const response = await fetch(`${API_BASE_URL}/compression/result/${jobId}`)

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
    const response = await fetch(`${API_BASE_URL}/compression/metrics/${jobId}`)

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
      `${API_BASE_URL}/compression/history?limit=${limit}`
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
    const response = await fetch(`${API_BASE_URL}/analytics/summary`)

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
    window.location.href = `${API_BASE_URL}/compression/download/${jobId}`
  },

  /**
   * Download original image
   * @param {string} jobId - The compression job ID
   */
  async downloadOriginal(jobId) {
    window.location.href = `${API_BASE_URL}/compression/download/${jobId}?type=original`
  },
}

export default apiClient
